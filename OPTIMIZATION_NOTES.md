# astlog 项目优化建议

> 范围：`include/`、`test.cc`、`CMakeLists.txt`（不含 `build/` 与 `.cache/` 生成物）

## P0（优先修复：正确性/稳定性）

1. `basicSink::setPattern_` 存在无限递归  
   - 位置：`include/sink/basic_sink-inl.h`  
   - 原因：函数体内再次调用自身，可能导致栈溢出。

2. `patternFormatter` 默认构造函数写法错误  
   - 位置：`include/pattern_formatter-inl.h`  
   - 原因：`patternFormatter("");` 构造的是临时对象，不是委托构造当前对象。

3. `ansicolorSink` 默认参数尝试实例化抽象类  
   - 位置：`include/sink/ansicolor_sink.h`  
   - 原因：`std::make_unique<details::formatter>()` 非法（抽象类不可实例化）。

4. 头文件中定义的自由函数缺少 `inline`，有 ODR 风险  
   - 位置：`include/loglevel-inl.h`、`include/logtime-inl.h`  
   - 原因：多个翻译单元包含后可能出现重复定义链接错误。

## P1（性能/并发）

5. 日志级别转字符串有不必要分配  
   - 位置：`include/loglevel-inl.h`  
   - 建议：使用 `constexpr` 固定映射（如数组/`string_view`）替代动态 `std::string` 构造。

6. `LoggerImpl::Logger()` 首次初始化并发不安全  
   - 位置：`include/log_line.h`  
   - 原因：`empty()` + `addSink()` 组合在并发首次调用时可能竞态。

7. `getLocalTime` 缓存与返回长度存在问题  
   - 位置：`include/logtime-inl.h`  
   - 原因：共享静态缓冲区在多线程下有数据竞争；返回长度使用 `sizeof(buf)` 会包含无效尾部。

8. `colorMap` 查找结构可优化  
   - 位置：`include/common.h`、`include/sink/ansicolor_sink-inl.h`  
   - 建议：若枚举值连续，改为数组下标映射，减少哈希查找开销。

## P2（可维护性/可移植性）

9. CMake 硬编码编译器路径  
   - 位置：`CMakeLists.txt`  
   - 原因：`set(CMAKE_CXX_COMPILER /usr/bin/g++)` 降低跨平台/CI 兼容性。

10. 头文件保护宏使用保留标识符前缀  
    - 示例：`ASTLOG_LOGGER_HPP`  
    - 建议：改为 `ASTLOG_LOGGER_HPP` 等非保留命名。

11. include 风格与目录暴露策略不统一  
    - 示例：`include/log_line.h` 直接包含 `ansicolor_sink.h`  
    - 建议：统一 `#include "sink/xxx.h"`，并尽量只暴露根 include 目录。

12. 存在占位/空头文件影响可读性  
    - 位置：`include/formatter.h`、`include/astlog.h`  
    - 建议：补全实现或移除，避免误导使用者。

---

## 建议落地顺序

1. 先修 P0（确保可用与稳定）  
2. 再做 P1（并发安全 + 热路径性能）  
3. 最后处理 P2（结构与工程化质量）

---

## 本轮复查新增待修问题（2026-09-27）

1. `getLocalTime` 时间缓存逻辑会“卡住”  
   - 位置：`include/logtime-inl.h`  
   - 原因：`thread_local auto now = clock::now();` 只初始化一次，后续不更新，导致缓存命中判断长期成立。

2. `LEVEL::MAX` 与颜色表长度可能不一致  
   - 位置：`include/loglevel.h`、`include/sink/color_sink.h`、`include/common.h`  
   - 原因：若 `toLevel("MAX")` 返回 `LEVEL::MAX`，而 `colorMap` 未给 `MAX` 配色，会产生越界风险。

3. `toStr` 中 `std::move(#x)` 不合理  
   - 位置：`include/loglevel-inl.h`  
   - 原因：`#x` 是字符串字面量，不需要 move；建议直接构造 `std::string` 或改为 `string_view` 查表。

4. `astlog.h` 仍是危险占位状态  
   - 位置：`include/astlog.h`  
   - 原因：存在空宏 `#define DEBUG() \`，可能导致预处理阶段吞并后续代码。
