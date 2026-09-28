#ifndef ASTLOG_LOG_LINE_H
#define ASTLOG_LOG_LINE_H

#include <pthread.h>
#include <memory>

#include "sink/ansicolor_sink.h"
#include "logger.h"
#include "common.h"

struct LoggerImpl{
    
    static astlog::Logger& Logger(){
        static astlog::Logger logger{"root"};
        static const bool initialized = [] (astlog::Logger& instance) {
            sink::sinkPtr sink = std::make_shared<sink::ansicolorSink<sink::NonMutex>>(
                defaultConfig::colorMap,
                std::make_unique<details::patternFormatter>(defaultConfig::formatStr));
            sink->setLevel(LEVEL::DEBUG);
            instance.addSink(std::move(sink));
            instance.setLevel(LEVEL::DEBUG);
            instance.setFlush(LEVEL::INFO);
            return true;
        }(logger);
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







#endif