#ifndef __ASTLOG_COMMON_HPP
#define __ASTLOG_COMMON_HPP
#include <chrono>
#include <cstddef>
#include <type_traits>



namespace details{


#ifndef ASTLOG_EOL
    #ifdef _WIN32
    #define ASTLOG_EOL "\r\n"
    #else
    #define ASTLOG_EOL "\n"
    #endif
#endif

constexpr static const char *EOL = ASTLOG_EOL;



template<class T>
requires std::is_integral_v<T>
size_t countDigit(T t)
{
    size_t count = 1;
    for(;;)
    {
        if(t<10) return count;
        if(t<100) return count + 1;
        if(t<1000) return count + 2;
        if(t<10000) return count + 3;
        t /= 10000u;
        count += 4;
    }
}


}








#endif