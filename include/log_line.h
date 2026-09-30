#ifndef ASTLOG_LOG_LINE_H
#define ASTLOG_LOG_LINE_H

#include <pthread.h>
#include <memory>

#include "sink/ansicolor_sink.h"
#include "logger.h"
#include "common.h"

struct LoggerImpl{
    
    static astlog::Logger& Logger(){
        static sink::sinkPtr sink = std::make_shared<sink::ansicolorSink<sink::NonMutex>>(
                defaultConfig::colorMap,
                std::make_unique<details::patternFormatter>(defaultConfig::formatStr));
        static astlog::Logger logger{"root",sink};
        static const bool initialized = [] (astlog::Logger& instance ,sink::sinkPtr sink) {
            sink->setLevel(LEVEL::DEBUG);
            instance.addSink(std::move(sink));
            instance.setLevel(LEVEL::DEBUG);
            instance.setFlush(LEVEL::INFO);
            return true;
        }(logger,sink);
        (void)initialized;
        return logger;
    }
};


#define TRACE(fmt,...)   LoggerImpl::Logger().trace(fmt __VA_OPT__(,)__VA_ARGS__)
#define DEBUG(fmt,...)   LoggerImpl::Logger().debug(fmt __VA_OPT__(,)__VA_ARGS__)
#define INFO(fmt,...)    LoggerImpl::Logger().info(fmt __VA_OPT__(,)__VA_ARGS__)
#define WARN(fmt,...)    LoggerImpl::Logger().warn(fmt __VA_OPT__(,)__VA_ARGS__)
#define ERROR(fmt,...)   LoggerImpl::Logger().error(fmt __VA_OPT__(,)__VA_ARGS__)
#define FATAL(fmt,...)   LoggerImpl::Logger().fatal(fmt __VA_OPT__(,)__VA_ARGS__)



#define ASYNC_TRACE(async_logger,fmt,...)   { static_assert(std::is_same_v<decltype(logger),std::shared_ptr<astlog::asyncLogger>>);\
                                              async_logger->trace(fmt __VA_OPT__(,)__VA_ARGS__);}
#define ASYNC_DEBUG(async_logger,fmt,...)   { static_assert(std::is_same_v<decltype(logger),std::shared_ptr<astlog::asyncLogger>>);\
                                              async_logger->debug(fmt __VA_OPT__(,)__VA_ARGS__);}
#define ASYNC_INFO(async_logger,fmt,...)    { static_assert(std::is_same_v<decltype(logger),std::shared_ptr<astlog::asyncLogger>>);\
                                              async_logger->info(fmt __VA_OPT__(,)__VA_ARGS__);}
#define ASYNC_WARN(async_logger,fmt,...)    { static_assert(std::is_same_v<decltype(logger),std::shared_ptr<astlog::asyncLogger>>);\
                                              async_logger->warn(fmt __VA_OPT__(,)__VA_ARGS__);}
#define ASYNC_ERROR(async_logger,fmt,...)   { static_assert(std::is_same_v<decltype(logger),std::shared_ptr<astlog::asyncLogger>>);\
                                              async_logger->error(fmt __VA_OPT__(,)__VA_ARGS__);}
#define ASYNC_FATAL(async_logger,fmt,...)   { static_assert(std::is_same_v<decltype(logger),std::shared_ptr<astlog::asyncLogger>>);\
                                              async_logger->fatal(fmt __VA_OPT__(,)__VA_ARGS__);}


#define SYNC_TRACE(async_logger,fmt,...)   { static_assert(std::is_same_v<decltype(logger),std::shared_ptr<astlog::Logger>>);\
                                              sync_logger->trace(fmt __VA_OPT__(,)__VA_ARGS__);}
#define SYNC_DEBUG(sync_logger,fmt,...)   { static_assert(std::is_same_v<decltype(logger),std::shared_ptr<astlog::Logger>>);\
                                              sync_logger->debug(fmt __VA_OPT__(,)__VA_ARGS__);}
#define SYNC_INFO(sync_logger,fmt,...)    { static_assert(std::is_same_v<decltype(logger),std::shared_ptr<astlog::Logger>>);\
                                              sync_logger->info(fmt __VA_OPT__(,)__VA_ARGS__);}
#define SYNC_WARN(sync_logger,fmt,...)    { static_assert(std::is_same_v<decltype(logger),std::shared_ptr<astlog::Logger>>);\
                                              sync_logger->warn(fmt __VA_OPT__(,)__VA_ARGS__);}
#define SYNC_ERROR(sync_logger,fmt,...)   { static_assert(std::is_same_v<decltype(logger),std::shared_ptr<astlog::Logger>>);\
                                              sync_logger->error(fmt __VA_OPT__(,)__VA_ARGS__);}
#define SYNC_FATAL(sync_logger,fmt,...)   { static_assert(std::is_same_v<decltype(logger),std::shared_ptr<astlog::Logger>>);\
                                              sync_logger->fatal(fmt __VA_OPT__(,)__VA_ARGS__);}




#endif