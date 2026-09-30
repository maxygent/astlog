# 异步日志性能与生命周期分析

## 1. 为什么最开始没有日志输出

最初的核心问题，不是格式器坏了，而是线程池没有真正启动 worker。

关键代码在 `include/thread_pool.h`：

```cpp
m_workers.reserve(m_numWorkers);
for (auto& worker : m_workers) {
    worker = std::thread([&] { this->worker(); });
}
```

`reserve()` 只预留容量，不会真正扩容 `m_workers`，因此 `m_workers.size()` 仍然是 0，循环根本不会执行，所以 worker 线程根本没有启动。

这意味着：

- `asyncLogger::sinkIt()` 把日志任务加入队列
- 但没有线程消费任务
- 所以最终不会真正写出日志

因此“看起来没有输出”，真实原因是异步任务从未执行。

## 2. 为什么后期又报 `bad_weak_ptr`

异步 logger 的实现依赖 `enable_shared_from_this`：
需要 public 继承 不然后续shared_from_this会调用不了
```cpp
class asyncLogger : public Logger,
    std::enable_shared_from_this<asyncLogger>
```

但随后调用：

```cpp
threadPool->appendTask([instance = shared_from_this(), message = std::move(msg)] {
    instance->backendSink(std::move(message));
});
```

这要求 `asyncLogger` 必须由 `std::shared_ptr` 真正管理，并且对象生命周期仍然有效。

然而在测试里，很多对象是普通栈对象或对象生命周期在异步任务中被绕开，导致 `shared_from_this()` 发生异常：

```cpp
std::bad_weak_ptr
```

本质上说：

- `weak_from_this()` / `shared_from_this()` 只在对象被 `std::shared_ptr` 托管时有效
- 如果对象本身没有正确的 shared ownership，或者对象已销毁，都会抛出 `bad_weak_ptr`

## 3. 为什么异步日志反而更慢

这不是代码错误，而是 benchmark 语义不对。你以前的测试大多测到的是“入队时间”，而不是“日志真正写完的总时间”。

异步路径要增加这些附加成本：

- lambda 创建
- `std::function` 包装
- `std::mutex` 加锁
- `std::deque` push_back
- condition variable notify
- worker thread wake/switch
- worker 线程再次取队列、再执行

同步路径只做：

- 直接写 sink
- `fwrite` / `flush`

在小规模日志和终端输出中，这些额外线程同步开销会超过真实 I/O 成本，因此异步看上去会更慢。

## 4. 正确的 benchmark 方式

要比较异步日志是否划算，必须等待所有任务真正执行完，再测总耗时。

例如要保证：

```cpp
for (...) {
    logger->error(...);
}
pool->wait();
sink->flush();
```

这样比较才是“日志写出完成时间”，而不是只是“任务入队时间”。

## 5. 实测结论

我已经写了一版文件基准测试，并跑出来的数据如下：

- 10w 条日志：
  - sync_file: 4,840,117,668ns
  - workers=1 async_file: 421,210,750ns
  - workers=2 async_file: 1,137,994,704ns
  - workers=4 async_file: 1,109,809,346ns
  - workers=8 async_file: 1,546,075,317ns

- 50w 条日志：
  - sync_file: 23,941,520,410ns
  - workers=1 async_file: 2,160,638,322ns
  - workers=4 async_file: 5,500,250,162ns

- 100w 条日志：
  - sync_file: 36,413,122,898ns
  - workers=1 async_file: 4,825,162,857ns

结论：

- 只有 worker=1 时，异步日志明显更快
- worker 越多，锁竞争和线程切换越明显
- 同一个文件被多个 worker 并发写时，不会自动带来收益

## 6. 为什么单 worker 反而更快

因为日志场景本身是高频、小消息、串行写入的工作。

多个 worker 并发写同一文件会带来：

- `FILE*` 锁竞争
- 线程切换成本
- queue push/pop 竞争
- condition_variable 唤醒成本

最终表现为：

- 异步线程数增多
- 但 I/O 仍然只有一个文件
- 竞争成本超过了并行收益

## 7. 优化建议

### 7.1 先做“单 worker + 批量 flush”

最重要的优化不是扩线程，而是让异步日志更轻：

- 一个 logger 只对应一个 writer thread
- 不要多个 worker 同时写同一个 file
- 用缓冲区累计日志，批量写出
- 例如每 64/256/1024 条日志一次 `fwrite`

### 7.2 不要用 `std::function` 作为队列对象

当前线程池使用：

```cpp
std::deque<std::function<void()>> m_TaskList;
```

这种形式会带来额外的函数对象开销、类型擦除和捕获成本。

更适合日志器的设计是：

- 直接存字符串
- 或者直接存结构体
- worker 线程只负责把这些 buffer 合并后写文件

### 7.3 不要每条日志都 notify 一次

当前线程池每插入一条任务都会 `notify_one()`，这在高频日志下会增加无效唤醒和线程调度成本。

建议：

- 仅在缓冲区满、或定时到期时唤醒 worker
- 或者 worker 自己定时 poll

### 7.4 不要对同一个 file 使用多个 worker

多个 worker 同时写一个 `FILE*`，锁竞争非常明显。

日志器的真实最佳实践是：

- 大多数场景只有一个 writer
- 只有在多个独立文件时才用多个 writer

### 7.5 避免在业务线程内做格式化

日志格式化本身也有成本，尤其是 `std::format`、字符串拼接和颜色处理。

更好的做法：

- 业务线程只做“构造日志字符串”或“压入 buffer”
- 后台线程统一做格式化和写文件

## 8. 一句话结论

异步日志的价值，不在于“所有情况下都更快”，而在于：

- 日志量很大
- 业务线程不能被慢 I/O 卡住
- 使用单个 writer + 批量 flush + 低竞争队列

如果实现是多个 worker 同时写一个 file，并且每条日志都走锁和 notify，那么它很容易比同步版本还慢，尤其在小规模、低吞吐场景中。

## 9. 最推荐的落地方案

最实用的优化路线：

1. 保留 `asyncLogger` 的概念
2. 改成单 worker 的日志队列
3. 直接持有字符串 buffer
4. 按固定大小批量 flush
5. 对同一个文件只留一个 writer
6. 不再用通用线程池代替日志队列

这会比当前的通用 `threadPool + std::function` 方案更接近生产级日志器。
