#ifndef __ASTLOG_LOGGER_HPP
#define __ASTLOG_LOGGER_HPP

#include <atomic>
#include <string>
#include <source_location>
#include <format>
#include <utility>


#include "loglevel.h"
#include "sink/sink.h"
#include "logmsg.h"


#ifdef ASTLOG_WCHAR_TO_UTF8_SUPPORT
#endif

namespace astlog{


template <typename T>
struct withSourceLocation {
   private:
    T inner;
    std::source_location loc;

   public:
    template <typename U>
        requires std::constructible_from<T, U>
    consteval withSourceLocation(
        U &&inner, std::source_location loc = std::source_location::current())
        : inner(std::forward<U>(inner)), loc(std::move(loc)) {}
    constexpr T const &format() const { return inner; }
    constexpr std::source_location const &location() const { return loc; }
};


class Logger{
public:
    explicit Logger(std::string name):m_name(std::move(name)){}
    template<class..._Args>
    void log(LEVEL level, withSourceLocation<std::format_string<_Args...>> fmt,_Args &&...args){
        if(shouldLog(level)){
            details::logmsg msg{ 
                .m_level = level,
                .m_loggerName = m_name,
                .m_loc = fmt.location(),
                .m_payload = std::format(fmt.format(),std::forward<_Args>(args)...),
            };
            sinkIt(std::move(msg));
        }
    }
    template<class..._Args>
    void trace( withSourceLocation<std::format_string<_Args...>> fmt,_Args &&...args){
        log(LEVEL::TRACE,fmt,std::forward<_Args>(args)...);
    }
    
    template<class..._Args>
    void debug(withSourceLocation<std::format_string<_Args...>> fmt,_Args &&...args){
        log(LEVEL::DEBUG,fmt,std::forward<_Args>(args)...);
    }
    
    template<class..._Args>
    void info(withSourceLocation<std::format_string<_Args...>> fmt,_Args &&...args){
        log(LEVEL::INFO,fmt,std::forward<_Args>(args)...);
    }
    template<class..._Args>
    void warn(withSourceLocation<std::format_string<_Args...>> fmt,_Args &&...args){
        log(LEVEL::WARN,fmt,std::forward<_Args>(args)...);
    }
    template<class..._Args>
    void error(withSourceLocation<std::format_string<_Args...>> fmt,_Args &&...args){
        log(LEVEL::ERROR,fmt,std::forward<_Args>(args)...);
    }
    template<class..._Args>
    void fatal(withSourceLocation<std::format_string<_Args...>> fmt,_Args &&...args){
        log(LEVEL::FATAL,fmt,std::forward<_Args>(args)...);
    }
    
    
    void setLevel(LEVEL level){
        m_level.store(level,std::memory_order_relaxed);
    }
    void setFlush(LEVEL level){
        m_flushLevel.store(level,std::memory_order_relaxed);
    }
    bool shouldLog(LEVEL level){
        return level >= m_level.load(std::memory_order_relaxed);
    }
    bool shouldFlush(LEVEL level){
        return level >= m_flushLevel.load(std::memory_order_relaxed);
    }
protected:
    
    void sinkIt(const details::logmsg& msg);
    void flush();
    std::vector<sinkPtr> m_sinks;
    std::string m_name{"root"}; 
    level_t m_level{LEVEL::INFO};
    level_t m_flushLevel{LEVEL::ERROR};

};



}


#endif