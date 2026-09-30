#include "logger.h"
#include "sink/sink.h"



namespace astlog{


Logger::Logger(std::string name , sink::sinkPtr sink):Logger{std::move(name),{std::move(sink)}}{}
Logger::Logger(std::string name , std::initializer_list<sink::sinkPtr> sinks):Logger{std::move(name),sinks.begin(),sinks.end()}{}



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
    