#include "logtime.h"
#include <chrono>
#include <string_view>

namespace details {
namespace time {

std::string getLocalTime(std::string_view format){
    static auto cachedTime = clock::now();
    auto now = clock::now();
    static char buf[64] = {'\0'};
    if(now - cachedTime <= std::chrono::nanoseconds(1'000'000'000) && buf[0] != '\0')
    {
        //cache hitted
        return {buf,sizeof(buf)};
    }
    cachedTime = now;
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm* local_tm = std::localtime(&now_c);
    
    std::strftime(buf,sizeof(buf),format.data(),local_tm);
    return {buf,sizeof(buf)};
}











}


}