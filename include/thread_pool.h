#ifndef ASTLOG_THREAD_POOL_H
#define ASTLOG_THREAD_POOL_H

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <vector>

#include <atomic>
#include <source_location>
#include <utility>
#include <thread>
#include <mutex>
#include "logmsg.h"
#include "sink/sink.h"


namespace details {

template <class Func, class... Args>
concept Callable = requires(Func fn, Args&&... args) {
    fn(std::forward<Args>(args)...);
};

struct async_msg{
    async_msg() = default;
    explicit async_msg(sink::sinkPtr sinker,std::source_location loc,std::string payload):m_sinker(sinker)
    {
        m_msg.m_loc = loc;
        m_msg.m_payload = payload;
    }
    async_msg(sink::sinkPtr sinker, const logmsg& msg):m_msg(msg),m_sinker(sinker ){}
    logmsg m_msg;
    sink::sinkPtr m_sinker;
};


class threadPool {
public:
    static threadPool& getInstance(size_t num = 10){
        static threadPool pool{num};
        return pool;
    }
    threadPool(size_t num);
    void postmsg(sink::sinkPtr sinker ,const logmsg& msg);
    ~threadPool(){}

private:
    threadPool(const threadPool&) = delete;
    threadPool(threadPool&&) = delete;
    threadPool& operator=(const threadPool&) = delete;
    threadPool& operator=(threadPool&&) = delete;

    struct  impl;
    std::shared_ptr<impl> m_impl;
};

}  // namespace details

#include "thread_pool-inl.h"

#endif