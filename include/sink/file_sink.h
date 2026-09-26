#ifndef __ASTLOG_FILE_SINK_H
#define __ASTLOG_FILE_SINK_H

#include <cstdio>
#include <memory>

#include "basic_sink.h"
#include "logmsg.h"
#include "pattern_formatter.h"

namespace sink {

template <Lockable Mutex> class fileSink : public basicSink<Mutex> {
public:
  explicit fileSink(const std::string &, std::unique_ptr<details::formatter>);

  fileSink(const fileSink &) = delete;
  fileSink(fileSink &&) = delete;
  fileSink &operator=(const fileSink &) = delete;
  fileSink &operator=(fileSink &&) = delete;

  ~fileSink() {
    if (m_file) {
      std::fclose(m_file);
    }
  }

private:

  void sinkIt(const details::logmsg &) final override;
  void flush_() final override;
  void sync_() final override;
  std::FILE *m_file;
};

} // namespace sink

#include "file_sink-inl.h"



#endif