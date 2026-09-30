#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <vector>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/async_logger.h>
#include <spdlog/details/thread_pool.h>

namespace {

std::chrono::nanoseconds benchSyncFile(const std::string &path, std::size_t count) {
  std::filesystem::remove(path);
  auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path, true);
  auto logger = std::make_shared<spdlog::logger>("sync", sink);
  logger->set_pattern("%v");
  logger->set_level(spdlog::level::debug);

  auto start = std::chrono::steady_clock::now();
  for (std::size_t i = 0; i < count; ++i) {
    logger->error("hello world {:=>10}", static_cast<int>(i));
  }
  logger->flush();
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::steady_clock::now() - start);
}

std::chrono::nanoseconds benchAsyncFile(const std::string &path, std::size_t count, std::size_t workers) {
  std::filesystem::remove(path);
  auto tp = std::make_shared<spdlog::details::thread_pool>(8192, workers);
  auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path, true);
  auto logger = std::make_shared<spdlog::async_logger>("async", sink, tp, spdlog::async_overflow_policy::block);
  logger->set_pattern("%v");
  logger->set_level(spdlog::level::debug);

  auto start = std::chrono::steady_clock::now();
  for (std::size_t i = 0; i < count; ++i) {
    logger->error("hello world {:=>10}", static_cast<int>(i));
  }
  logger->flush();
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::steady_clock::now() - start);
}

} // namespace

int main() {
  const std::vector<std::size_t> counts = {100000, 500000, 1000000};
  const std::vector<std::size_t> workers = {1, 2, 4, 8};

  for (std::size_t count : counts) {
    auto sync_ns = benchSyncFile("spdlog_sync.log", count);
    std::cout << "count=" << count << " sync_file:" << sync_ns.count() << "ns\n";
    for (std::size_t worker : workers) {
      auto async_ns = benchAsyncFile("spdlog_async.log", count, worker);
      std::cout << "  workers=" << worker << " async_file:" << async_ns.count() << "ns\n";
    }
  }
}