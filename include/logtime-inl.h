#include "logtime.h"
#include <chrono>
#include <string_view>

namespace details {
namespace time {

std::string getLocalTime(std::string_view format){
    static thread_local auto cachedTime = clock::now();
    thread_local auto now = clock::now();
    now = clock::now();
    static thread_local  char buf[64] = {'\0'};
    if(now - cachedTime <= std::chrono::nanoseconds(1'000'000'000) && buf[0] != '\0')
    {
        //cache hitted
        return {buf,sizeof(buf)};
    }
    cachedTime = now;
    std::time_t now_c = std::chrono::system_clock::to_time_t(cachedTime);
    std::tm* local_tm = std::localtime(&now_c);
    
    size_t size = std::strftime(buf,sizeof(buf),format.data(),local_tm);
    return {buf,size};
}











}

}