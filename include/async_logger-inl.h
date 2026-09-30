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
        // 保留一个计数 有消息存在的情况下不销毁
        // 但是通常不会使用局部的Logger？
        // if(weak_from_this().lock())
        // {
        threadPool->appendTask([instance = shared_from_this(),message = std::move(msg)]{
        instance->backendSink(std::move(message));
        });
        // }
        // else {
        //     threadPool->appendTask([instance = this,message = std::move(msg)]{
        //     instance->backendSink(std::move(message));
        //     });
        // }
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
