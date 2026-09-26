#include "sink/basic_sink.h"
#include "logmsg.h"
#include "pattern_formatter.h"
#include <memory>
#include <mutex>

namespace sink {

template <Lockable Mutex>
basicSink<Mutex>::basicSink(std::unique_ptr<details::formatter> formatter)
    : m_formatter(std::move(formatter)) {}

template <Lockable Mutex>
basicSink<Mutex>::basicSink()
    : m_formatter(std::make_unique<details::patternFormatter>()) {}

template <Lockable Mutex>
void basicSink<Mutex>::log(const details::logmsg &msg) {
  std::lock_guard<Mutex> lock(m_mtx);
  sinkIt(msg);
}

template <Lockable Mutex> void basicSink<Mutex>::flush() {
  std::lock_guard<Mutex> lock(m_mtx);
  flush_();
}

template <Lockable Mutex> void basicSink<Mutex>::sync() {
  std::lock_guard<Mutex> lock(m_mtx);
  sync_();
}

template <Lockable Mutex>
void basicSink<Mutex>::setFormatter(
    std::unique_ptr<details::formatter> formatter) {
  std::lock_guard<Mutex> lock(m_mtx);
  setFormatter_(std::move(formatter));
}

template <Lockable Mutex>
void basicSink<Mutex>::setPattern(const std::string &pattern) {
  std::lock_guard<Mutex> lock(m_mtx);
  setPattern_(pattern);
}

template <Lockable Mutex>
void basicSink<Mutex>::setFormatter_(
    std::unique_ptr<details::formatter> formatter) {
  m_formatter = std::move(formatter);
}

template <Lockable Mutex>
void basicSink<Mutex>::setPattern_(const std::string &pattern) {
  std::lock_guard<Mutex> lock(m_mtx);
  setPattern_(pattern);
}

} // namespace sink




