#ifndef ASTLOG_ASYNC_LOGGER_H
#define ASTLOG_ASYNC_LOGGER_H


#include <memory>
#include "logger.h"
#include "logmsg.h"
#include "thread_pool.h"

namespace astlog{


class asyncLogger: public Logger,public std::enable_shared_from_this<asyncLogger>{

public:
    template <class It>
    asyncLogger(std::string name,It begin,It end,std::weak_ptr<details::threadPool> pool):
    Logger{std::move(name),begin,end},m_pool(pool){}

    asyncLogger(std::string name,sink::sinkPtr sink,std::weak_ptr<details::threadPool> pool);

    asyncLogger(std::string name,std::initializer_list<sink::sinkPtr> sinks,std::weak_ptr<details::threadPool> pool);




protected:
    void sinkIt(const details::logmsg &) override;
    void backendSink(const details::logmsg&);

    std::weak_ptr<details::threadPool> m_pool;

};

}



#include "async_logger-inl.h"

#endif