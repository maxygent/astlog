#ifndef ASTLOG_BASIC_SINK_H
#define ASTLOG_BASIC_SINK_H

#include <cstdio>

#include "logmsg.h"
#include "pattern_formatter.h"
#include "sink.h"

namespace sink {

template <Lockable Mutex> class basicSink : public sink {
public:
  basicSink();
  explicit basicSink(details::formatterPtr);

  basicSink(const basicSink &) = delete;
  basicSink(basicSink &&) = delete;
  basicSink &operator=(const basicSink &) = delete;
  basicSink &operator=(basicSink &&) = delete;

  void log(const details::logmsg &) final override;
  void flush() final override;
  void sync() final override;
  void setPattern(const std::string &) final override;
  void setFormatter(details::formatterPtr) final override;

  ~basicSink() {}

protected:
  virtual void flush_() = 0;
  virtual void sync_() = 0;
  virtual void setPattern_(const std::string &);
  virtual void setFormatter_(details::formatterPtr);

  details::formatterPtr m_formatter;
  Mutex m_mtx;
};

} // namespace sink

#include "basic_sink-inl.h"

#endif