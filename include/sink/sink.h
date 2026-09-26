#ifndef __ASTLOG_SINK_HPP
#define __ASTLOG_SINK_HPP
#include "logmsg.h"
#include "pattern_formatter.h"
#include <atomic>

namespace sink {
template <class Mutex>
concept Lockable = requires(Mutex &mtx) {
  { mtx.lock() } -> std::same_as<void>;
  { mtx.unlock() } -> std::same_as<void>;
  { mtx.try_lock() } -> std::convertible_to<bool>;
};
class sink {
public:
  bool shouldLog(astlog::LEVEL level) {
    return level >= m_level.load(std::memory_order_relaxed);
  }

  virtual void log(const details::logmsg &) = 0;
  virtual void flush() = 0;
  virtual void sync() = 0;
  virtual void setPattern(const std::string &) = 0;
  virtual void setFormatter(std::unique_ptr<details::formatter>) = 0;

  void setLevel(astlog::LEVEL level) {
    m_level.store(level, std::memory_order_relaxed);
  }
  astlog::LEVEL level() const {
    return m_level.load(std::memory_order_relaxed);
  }
  virtual ~sink() = default;

protected:
  virtual void sinkIt(const details::logmsg &) = 0;
  astlog::level_t m_level{astlog::LEVEL::INFO};
};

using sinkPtr = std::shared_ptr<sink>;

} // namespace sink

#endif