#ifndef __ASTLOG_ANSICOLOR_SINK_H
#define __ASTLOG_ANSICOLOR_SINK_H

#include <unordered_map>
#include "basic_sink.h"
#include "logmsg.h"
// #include "logger.h"


namespace sink {

// Formatting codes

template <Lockable Mutex> 
class ansicolorSink : public basicSink<Mutex> {
  public:
  ansicolorSink();
  ansicolorSink( std::unordered_map<LEVEL,std::string> colorMap,std::unique_ptr<details::formatter> formatter = std::make_unique<details::formatter>(),FILE *target_file = stdout);
  ~ansicolorSink() override = default;

  ansicolorSink(const ansicolorSink &other) = delete;
  ansicolorSink(ansicolorSink &&other) = delete;

  ansicolorSink &operator=(const ansicolorSink &other) = delete;
  ansicolorSink &operator=(ansicolorSink &&other) = delete;

  void setColor(LEVEL level, std::string color);
  void setColor(std::initializer_list<std::pair<LEVEL,std::string>> list);

  protected:
  void sinkIt(const details::logmsg&) override;
  void flush_() override;
  void sync_() override;
  std::unordered_map<LEVEL,std::string> m_colorMap;
  FILE* m_file;
};

} // namespace sink

#include "ansicolor_sink-inl.h"

#endif