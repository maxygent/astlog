#include "async_logger.h"
#include "logmsg.h"





namespace astlog {


asyncLogger::asyncLogger(std::string name,sink::sinkPtr sink,std::weak_ptr<details::threadPool> pool):
asyncLogger{std::move(name),{sink},pool}{}

asyncLogger::asyncLogger(std::string name,std::initializer_list<sink::sinkPtr> sinks,std::weak_ptr<details::threadPool> pool):
asyncLogger{std::move(name),sinks.begin(),sinks.end(),pool}{}


void asyncLogger::sinkIt(const details::logmsg& msg)
{
    if(auto threadPool = m_pool.lock();threadPool)
    {
        for(auto &sinker:m_sinks)
        {
            if(sinker->shouldLog(msg.m_level))
                threadPool->postmsg(sinker,msg);
        }
        
    }
}

void asyncLogger::backendSink(const details::logmsg& msg){
    for(auto &sink : Logger::m_sinks)
    {
        if(sink->shouldLog(msg.m_level))
        {
            sink->log(msg);
        }
    }
}

}
