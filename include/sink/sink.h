#ifndef __ASTLOG_SINK_HPP
#define __ASTLOG_SINK_HPP
#include <atomic>
#include "logmsg.h"

namespace astlog{
class sink{
    public:
    virtual void sinkIt(details::logmsg msg) = 0;
    bool shouldLog(LEVEL level){
        return level >= m_level.load(std::memory_order_relaxed);
    }
    virtual void log(const details::logmsg &msg) = 0;
    virtual void flush() = 0;
    virtual void set_pattern(const std::string &pattern) = 0;
    // virtual void set_formatter(std::unique_ptr<formatter> sink_formatter) = 0;
    
    void setLevel(LEVEL level){
        m_level.store(level,std::memory_order_relaxed);
    }
    LEVEL level() const {
        return m_level.load(std::memory_order_relaxed);
    }
    virtual ~sink() = default;
    protected:
    level_t m_level{LEVEL::INFO};
};

using sinkPtr = std::shared_ptr<sink>;


}


#endif