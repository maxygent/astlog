# astlog 性能和稳定性优化方案

## 1. 当前项目的核心问题概览

从代码结构来看，当前项目的“日志系统”主要分成 5 层：

1. Logger API 层：`include/logger.h`
2. Async Logger 层：`include/async_logger.h` 和 `include/async_logger-inl.h`
3. 线程池层：`include/thread_pool.h`
4. Sink 层：`include/sink/*`
5. Buffer / 格式化层：`include/membuf.h`、`include/pattern_formatter-inl.h`

目前实现的主要问题，不是单点 bug，而是“正确性”和“性能”两个方向同时存在：

- 对象生命周期不稳定：`asyncLogger` 依赖 `enable_shared_from_this`，但并不是所有创建方式都满足 shared ownership
- 线程池过通用：`std::function` + `std::deque` + `condition_variable` 的开销大，适合通用任务，不是日志 hot path
- 多 worker 同时写一个文件：会导致锁竞争和写入串行化，性能会被压制
- 每条日志都走缓冲与 flush：`fileSink` 里写入是逐条 `fwrite`，吞吐很低
- 缓冲区边界处理不安全：`membuf.h` 里存在越界写入风险
- 格式化开销高：`std::format` 与 `std::string` 反复生成/复制会带来明显成本

下面给出一份“针对该项目的具体优化方案”。

---

## 2. 先修稳定性，再谈性能

### 2.1 修复 async logger 的对象生命周期

现状问题：

- `asyncLogger` 继承了 `std::enable_shared_from_this<asyncLogger>`
- 但很多创建方式仍然允许裸指针/临时对象/局部对象参与异步任务
- 任务中有 `shared_from_this()`，一旦对象没有真实 shared ownership，就会抛出 `bad_weak_ptr`

优化建议：

- 统一用 `std::shared_ptr<asyncLogger>` 创建
- 异步任务里不要依赖 `shared_from_this()`
- 直接捕获 `std::shared_ptr<asyncLogger>`，不要捕获 `this`
- 在 `Logger` 基类增加虚析构函数，避免多态下的对象生命周期问题

推荐写法：

```cpp
auto logger = std::make_shared<astlog::asyncLogger>(
    "root", sink, weakPool);

pool->enqueue([logger, msg = std::move(msg)]() mutable {
    logger->backendSink(std::move(msg));
});
```

不要用：

```cpp
[instance = shared_from_this(), ...]
```

因为这要求对象必须严格由 shared_ptr 托管，且对象不能在异步阶段被销毁。

---

### 2.2 修复缓冲区越界问题

`include/membuf.h` 中 `inlineBuffer::erase()` 和 `append()` 有明显问题：

- `m_inline[m_size]` 在 `m_size == INLINE_CAPACITY` 时越界
- `m_overflow[m_size]` 在 `resize(m_size)` 后再访问 `m_size` 位置属于尾后写入
- `Logbuffer::write()` 的递归写法在 `size >= LogBufferSize` 时会导致无限递归

建议修正：

- `erase()` 只允许写到有效范围内，例如 `m_size < capacity`
- `append()` 中要先检查 `m_size + size <= capacity`
- 超长数据直接一次 `fwrite`，不要递归
- `flush()` 需要判断 `m_size == 0`，避免空写

建议的安全版伪代码：

```cpp
void write(const char* data, size_t size) {
    if (size == 0) return;
    if (size >= LogBufferSize) {
        flush();
        fwrite(data, 1, size, m_file);
        return;
    }
    if (m_size + size > LogBufferSize) {
        flush();
    }
    memcpy(buffer.data() + m_size, data, size);
    m_size += size;
}
```

---

## 3. 让线程池从“通用任务池”变成“日志专用队列”

### 3.1 当前问题

`include/thread_pool.h` 现在使用的是：

- `std::deque<std::function<void()>>`
- `std::mutex`
- `condition_variable_any`

这对于通用任务处理并不差，但对日志来说，成本过高：

- 每次任务都要打包成 `std::function`
- 每次都要锁队列
- 每次都要唤醒线程
- 线程切换成本不低

日志是高频、短任务、很多小对象，通用线程池明显不够“轻”。

### 3.2 更适合日志的方案

日志推荐用“单生产者 / 单消费者 + ring buffer”模型：

- 业务线程只负责把日志字符串写入 ring buffer
- 后台线程只负责 drain buffer 并写到 sink
- 一个 logger 对应一个 writer thread
- 不要多个 worker 同时写一个 file

这比 `std::deque<std::function<void()>>` 更适合：

- 更低的锁竞争
- 更少的 heap allocation
- 更好的缓存局部性
- 更可控的批量 flush

---

## 4. 把 sink 设计成“批量 flush”，而不是“逐条 flush”

### 4.1 当前问题

`fileSink` 中的 `sinkIt()` 逻辑会逐条调用 `fwrite`：

```cpp
fwrite(buf.data(), sizeof(char), buf.size(), m_file);
fwrite(EOL, sizeof(char), 1, m_file);
```

如果每条日志都写一次，性能大幅下降，尤其是在：

- 低吞吐日志
- 文件输出
- 高频 `error/debug` 场景

### 4.2 改造建议

文件 sink 应该缓存一批日志，再一次性写出：

- 例如 64/128/256 条日志之后 flush
- 或者在缓冲区满时 flush
- 或者按时间窗口 flush（例如 10ms/50ms）

推荐结构：

```cpp
class file_sink {
    std::string buffer;
    std::mutex mutex;
    std::thread writer;
    std::condition_variable cv;
    bool stop = false;

    void append(const std::string& msg) {
        std::lock_guard<std::mutex> lock(mutex);
        buffer += msg;
        if (buffer.size() > 64 * 1024) {
            flush_locked();
        }
    }

    void flush_locked() {
        fwrite(buffer.data(), 1, buffer.size(), fp);
        buffer.clear();
    }
};
```

这会使日志吞吐量显著提升，并减少 `fwrite` 次数。

---

## 5. 格式化层应该从 std::format 转向 fmt

### 5.1 当前问题

`include/logger.h` 中的日志 API 直接使用：

```cpp
std::format(fmt.format(), std::forward<_Args>(args)...)
```

这在高频日志场景下并不理想：

- 生成格式化字符串和参数包的开销大
- `std::format` 还要处理更严格的类型和格式规则
- 语义上比 spdlog 的 `fmt` 更重

### 5.2 建议

- 优先改用 `fmt` 或 `fmt::format_to` 作为格式化核心
- 将 `pattern_formatter` 做成“格式化时 append 到 buffer”而不是反复创建临时字符串
- 把 format 逻辑实现在 `formatterBuf` 上，减少中间 string 分配

例如：

```cpp
fmt::format_to(std::back_inserter(dest), "{}", value);
```

比在 `std::string` 上反复 `+` 更高效。

---

## 6. 缩小 object lifetime 和跨线程访问的风险

### 6.1 当前问题

- `Logger` 里有 `m_sinks` 和 `m_level`
- `asyncLogger` 重写 `sinkIt` 并且异步执行 `backendSink`
- 线程池任务可能访问已销毁对象
- 某些对象的析构不受控制

### 6.2 建议

- `Logger` 增加 `virtual ~Logger() = default;`
- `asyncLogger` 不再用 `enable_shared_from_this` 这种复杂方法
- 用 `std::shared_ptr` 显式传递给异步任务
- 任务结束后，所有 `shared_ptr` 退出作用域，生命周期自然结束

---

## 7. 对单个 file 的并发写，必须控制并发度

### 7.1 当前问题

项目中的测试里，多个 worker 同时写同一个文件，最终会导致：

- `FILE*` 竞争
- queue lock 竞争
- 文件写入串行化
- 总耗时不降反升

### 7.2 优化建议

- 同一个文件只允许一个 writer
- 如果必须多个 writer，只允许每个 writer 写不同文件
- 也就是说，日志路径设计上尽量是：
  - 一个 logger -> 一个 sink -> 一个 file
  - 否则性能会明显下降

---

## 8. 一个更适合 astlog 的目标架构

建议最终形态：

### 8.1 Logger API

```cpp
astlog::Logger logger{"root"};
logger.set_level(LEVEL::INFO);
logger.info("user {} login", user_id);
```

### 8.2 Sink API

```cpp
auto file_sink = std::make_shared<sink::file_sink>("app.log");
logger.add_sink(file_sink);
```

### 8.3 Async writer

- 只保留一个后台 worker
- 一个文件对应一个 writer
- 使用 ring buffer / batch buffer
- 不要 `std::function` 做队列元素

### 8.4 推荐队列结构

```cpp
struct LogEntry {
    std::string message;
    LEVEL level;
    std::chrono::steady_clock::time_point ts;
};

std::vector<LogEntry> batch;
```

后台线程每次从 queue 中批量取出若干条记录，拼接成一个大 block，再一次性写入文件。

这种结构比 `std::function<void()>` 更适合高频日志应用。

---

## 9. 实施顺序建议

### 第一阶段：稳定性修复

1. 修复 `membuf.h` 边界写入问题
2. 修复 `asyncLogger` 的对象生命周期
3. 给 `Logger` 增加虚析构函数
4. 统一所有异步任务使用 `std::shared_ptr` 显式捕获

### 第二阶段：性能优化

1. 删除通用 `std::function` 任务队列
2. 改成单 worker + batch buffer
3. `fileSink` 改为缓存一批后统一 `fwrite`
4. 避免多 worker 同时写一个文件

### 第三阶段：格式化优化

1. 把 `std::format` 换成 `fmt`
2. 让 formatter 直接 append 到 buffer，而不是反复生成中间字符串
3. 限制不必要的 pad 和 truncation 逻辑

### 第四阶段：benchmark 验证

1. 同步 vs 异步写文件
2. 单 worker vs 多 worker
3. 1e5 / 5e5 / 1e6 日志量
4. 不同 sink 场景：stdout / file / null sink

---

## 10. 一句话总结

项目最重要的优化方向不是“堆更多线程”，而是：

- 保证生命周期正确
- 避免越界和竞争
- 把日志收敛为“单 writer + 批量 flush + 低锁设计”
- 把 hot path 从 `std::function` / `std::format` / 每条写文件，改成 buffer + compact queue + batch I/O

如果按这个方案落地，astlog 的性能会从“教学版示例”提升到更接近工程级日志库的水准。