#ifndef __ASTLOG_LOGTIME_H
#define __ASTLOG_LOGTIME_H
#include <chrono>
#include <ctime>
#include <string_view>


namespace details {

using clock = std::chrono::system_clock;
namespace time{
std::string getLocalTime(std::string_view format = "%Y-%m-%d  %H:%M:%S");




}
}

#include "logtime-inl.h"


#endif