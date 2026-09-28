#ifndef ASTLOG_ANSICOLOR_SINK_H
#define ASTLOG_ANSICOLOR_SINK_H


#include "basic_sink.h"
#include "logmsg.h"
#include "pattern_formatter.h"
// #include "logger.h"


namespace sink {

// Formatting codes

template <Lockable Mutex> 
class ansicolorSink : public basicSink<Mutex> {
  public:
  ansicolorSink();
  ansicolorSink( colorMap color,details::formatterPtr formatter = std::make_unique<details::patternFormatter>(),FILE *target_file = stdout);
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
  colorMap  m_colorMap;
  FILE* m_file;
};

} // namespace sink

#include "ansicolor_sink-inl.h"

#endif