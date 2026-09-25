#ifndef __ASTLOG_logmsg_HPP
#define __ASTLOG_logmsg_HPP

#include <source_location>
#include <string_view>
#include <pthread.h>
#include <sys/syscall.h>

#include "loglevel.h"
#include "logtime.h"

namespace details{
    
struct logmsg{        
    inline static const clock::time_point m_timeStart = clock::now();
    astlog::LEVEL m_level{astlog::LEVEL::TRACE};
    clock::time_point m_timeStamp{clock::now()};
    std::string_view m_loggerName{""};
    // clock::duration m_timeElapsed;
    size_t m_threadId{static_cast<size_t>(::syscall(SYS_gettid))};
    
    mutable size_t m_colorRangeStart{0};
    mutable size_t m_colorRangeStop{0};

    std::source_location m_loc;
    std::string_view m_payload{""};
};
    
    

}





#endif