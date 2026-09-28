#ifndef ASTLOG_LOGLEVEL_HPP
#define ASTLOG_LOGLEVEL_HPP

#include <atomic>
#include <cstring>
#include <string>
#include <type_traits>


#define FOREACH(f) f(TRACE) f(DEBUG) f(INFO) f(WARN) f(ERROR) f(FATAL) f(MAX)

enum class LEVEL : unsigned char {
#define LOGNAME(name) name,
  FOREACH(LOGNAME)
#undef LOGNAME
};

inline static constexpr size_t LEVEL_MAX = std::underlying_type_t<LEVEL>(LEVEL::MAX);
constexpr std::string toStr(LEVEL level);
constexpr LEVEL toLevel(std::string_view level);
constexpr LEVEL toLevel(std::string level);
constexpr size_t toIndex(LEVEL level);

using level_t = std::atomic<LEVEL>;
 // namespace astlog

#include "loglevel-inl.h"

#endif