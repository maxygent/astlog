#ifndef __ASTLOG_LOGLEVEL_HPP
#define __ASTLOG_LOGLEVEL_HPP

#include <atomic>
#include <cstring>
#include <string>

namespace astlog{
#define FOREACH(f) \
    f(TRACE) f(DEBUG) f(INFO) f(WARN) f(ERROR) f(FATAL)

enum class LEVEL : unsigned char{
#define LOGNAME(name) name,
    FOREACH(LOGNAME)
#undef LOGNAME
};

std::string toStr(LEVEL level);
LEVEL toLevel(std::string_view level);
LEVEL toLevel(std::string level);

using level_t = std::atomic<astlog::LEVEL>;
}








#endif