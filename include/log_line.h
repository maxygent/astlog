#ifndef __ASTLOG_LOG_LINE_H
#define __ASTLOG_LOG_LINE_H

#include <pthread.h>
#include <memory>
#include "ansicolor_sink.h"
#include "logger.h"
#include "sink.h"
#include "common.h"

struct LoggerImpl{
    
    static astlog::Logger& Logger(){
        static astlog::Logger __Logger{"root"};
        if(__Logger.empty())
        {
            sink::sinkPtr sink = std::make_shared<sink::ansicolorSink<sink::NonMutex>>(defaultConfig::colorMap,
                std::make_unique<details::patternFormatter>(defaultConfig::formatStr));
                sink->setLevel(LEVEL::DEBUG);
            __Logger.addSink(std::move(sink));
            __Logger.setLevel(LEVEL::DEBUG);
            __Logger.setFlush(LEVEL::INFO);
        }
        return __Logger;
    }
};


#define TRACE(fmt,...)   LoggerImpl::Logger().trace(fmt __VA_OPT__(,)__VA_ARGS__)
#define DEBUG(fmt,...)   LoggerImpl::Logger().debug(fmt __VA_OPT__(,)__VA_ARGS__)
#define INFO(fmt,...)    LoggerImpl::Logger().info(fmt __VA_OPT__(,)__VA_ARGS__)
#define WARN(fmt,...)    LoggerImpl::Logger().warn(fmt __VA_OPT__(,)__VA_ARGS__)
#define ERROR(fmt,...)   LoggerImpl::Logger().error(fmt __VA_OPT__(,)__VA_ARGS__)
#define FATAL(fmt,...)   LoggerImpl::Logger().fatal(fmt __VA_OPT__(,)__VA_ARGS__)







#endif