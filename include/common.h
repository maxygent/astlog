#ifndef __ASTLOG_COMMON_HPP
#define __ASTLOG_COMMON_HPP
#include <cstddef>
#include <type_traits>
#include <unordered_map>

#include "sink/color_sink.h"
#include "loglevel.h"

namespace defaultConfig{
const static inline std::unordered_map<LEVEL,std::string> colorMap = {
    {LEVEL::TRACE,sink::blue + sink::on_white},
    {LEVEL::DEBUG,sink::white},
    {LEVEL::INFO,sink::green + sink::on_cyan},
    {LEVEL::WARN,sink::yellow_bold + sink::on_black},
    {LEVEL::ERROR,sink::red},
    {LEVEL::FATAL,sink::red+sink::on_white}};

const static inline std::string formatStr = "%d%m %s [%t] [%l] [%n] [%e] [%v]";


}




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

// class Mutex{
// public:
//     Mutex(std::mutex mtx){}
    
//     ~Mutex(){}
// private:
//     std::mutex m_mtx;
// };




}








#endif