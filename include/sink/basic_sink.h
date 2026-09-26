#ifndef __ASTLOG_BASIC_SINK_H
#define __ASTLOG_BASIC_SINK_H

#include <concepts>
#include <cstdio>
#include <memory>

#include "logmsg.h"
#include "pattern_formatter.h"
#include "sink.h"

namespace sink {

template <Lockable Mutex> class basicSink : public sink {
public:
  basicSink();
  explicit basicSink(std::unique_ptr<details::formatter>);

  basicSink(const basicSink &) = delete;
  basicSink(basicSink &&) = delete;
  basicSink &operator=(const basicSink &) = delete;
  basicSink &operator=(basicSink &&) = delete;

  void log(const details::logmsg &) final override;
  void flush() final override;
  void sync() final override;
  void setPattern(const std::string &) final override;
  void setFormatter(std::unique_ptr<details::formatter>) final override;

  ~basicSink() {}

protected:
  virtual void flush_() = 0;
  virtual void sync_() = 0;
  virtual void setPattern_(const std::string &);
  virtual void setFormatter_(std::unique_ptr<details::formatter>);

  std::unique_ptr<details::formatter> m_formatter;
  Mutex m_mtx;
};

} // namespace sink

#include "basic_sink-inl.h"

#endif