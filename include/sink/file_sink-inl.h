#include <cstdio>
#include <memory>
#include <syncstream>
#include <unistd.h>

#include "logmsg.h"
#include "membuf.h"
#include "pattern_formatter.h"
#include "sink/basic_sink.h"
#include "sink/file_sink.h"


namespace sink {

template <Lockable Mutex>
fileSink<Mutex>::fileSink(const std::string &fileName,
                          std::unique_ptr<details::formatter> formatter)
    : basicSink<Mutex>(std::move(formatter)) {

  m_file = std::fopen(fileName.c_str(), "ab");
  if (!m_file) {
    perror("File not exists or access denied");
  }
}
template <Lockable Mutex>
void fileSink<Mutex>::sinkIt(const details::logmsg &msg) {
  details::formatterBuf buf;
  basicSink<Mutex>::m_formatter->format(msg,buf);
  std::fwrite(buf.data(), sizeof(char), buf.size(), m_file);
}

template <Lockable Mutex> void fileSink<Mutex>::flush_() {
  std::fflush(m_file);
}

template <Lockable Mutex> void fileSink<Mutex>::sync_() {
  ::fdatasync(::fileno(m_file));
}

} // namespace sink


// 工厂模式
// template <typename Factory = synchronous_factory>
// inline std::shared_ptr<logger> basic_logger_mt(const std::string
// &logger_name,
//                                                const filename_t &filename,
//                                                bool truncate = false,
//                                                const file_event_handlers
//                                                &event_handlers = {}) {
//     return Factory::template create<sinks::basic_file_sink_mt>(logger_name,
//     filename, truncate,
//                                                                event_handlers);
// }

// template <typename Factory = synchronous_factory>
// inline std::shared_ptr<logger> basic_logger_st(const std::string
// &logger_name,
//                                                const filename_t &filename,
//                                                bool truncate = false,
//                                                const file_event_handlers
//                                                &event_handlers = {}) {
//     return Factory::template create<sinks::basic_file_sink_st>(logger_name,
//     filename, truncate,
//                                                                event_handlers);
// }
