#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <thread>

#include "common.h"
#include "log_line.h"
#include "pattern_formatter.h"
#include "thread_pool.h"
#include "async_logger.h"
#include "sink/file_sink.h"

namespace {

std::chrono::nanoseconds benchSyncFile(const std::string &path, std::size_t count) {
  std::filesystem::remove(path);
  auto sink = std::make_shared<sink::fileSink<std::mutex>>(
      path, std::make_unique<details::patternFormatter>(defaultConfig::formatStr));
  astlog::Logger logger{"root", sink};
  logger.setLevel(LEVEL::DEBUG);

  auto start = std::chrono::steady_clock::now();
  for (std::size_t i = 0; i < count; ++i) {
    logger.error("hello world {:=^10}", static_cast<int>(i));
  }
  sink->flush();
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::steady_clock::now() - start);
}

std::chrono::nanoseconds benchAsyncFile(const std::string &path,
                                       std::size_t count,
                                       std::size_t workers) {
  std::filesystem::remove(path);
  auto pool = std::make_shared<details::threadPool>(workers);
  auto sink = std::make_shared<sink::fileSink<std::mutex>>(
      path, std::make_unique<details::patternFormatter>(defaultConfig::formatStr));
  auto logger = std::make_shared<astlog::asyncLogger>(
      "root", sink, std::weak_ptr<details::threadPool>(pool));

  auto start = std::chrono::steady_clock::now();
  for (std::size_t i = 0; i < count; ++i) {
    logger->error("hello world {:=^10}", static_cast<int>(i));
  }
  sink->flush();
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::steady_clock::now() - start);
}

} // namespace

int main() {
  const std::vector<std::size_t> counts = {100000, 500000, 1000000};
  const std::vector<std::size_t> workers = {1};

  for (std::size_t count : counts) {
    auto sync_ns = benchSyncFile("sync_bench.log", count);
    std::cout << "count=" << count << " sync_file:" << sync_ns.count() << "ns\n";

    for (std::size_t worker : workers) {
      auto async_ns = benchAsyncFile("async_bench.log", count, worker);
      std::cout << "  workers=" << worker << " async_file:" << async_ns.count() << "ns\n";
    }
  }
}



