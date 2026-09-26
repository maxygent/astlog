#include "logger.h"



namespace astlog{
void Logger::sinkIt(const details::logmsg& msg){
    for(auto &sink : m_sinks)
    {
        if(sink->shouldLog(msg.m_level))
        {
            sink->log(msg);
        }
    }
    if(shouldFlush(msg.m_level))
    {
        flush();
    }
}


void Logger::flush(){
    for(auto &sink : m_sinks)
    {
        sink->flush();
    }
}

}
    