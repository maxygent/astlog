#ifndef ASTLOG_THREAD_POOL_H
#define ASTLOG_THREAD_POOL_H

#include <condition_variable>
#include <cstddef>

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

struct async_msg : public logmsg{
    explicit async_msg(sink::sinkPtr sinker,std::source_location loc,std::string pauload):m_sinker(sinker)
    {
        logmsg::m_loc = loc;
        logmsg::m_payload = pauload;
    }
    async_msg(sink::sinkPtr sinker, const logmsg& msg):logmsg(msg),m_sinker(sinker ){}

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
// class threadPool {
// public:
//     static threadPool& getInstance(size_t num = 10){
//         static threadPool pool{num};
//         return pool;
//     }

//     threadPool(size_t num):m_numWorkers(num)
//      {
//         m_workers.resize(m_numWorkers);
//         for (auto& worker : m_workers) {
//             worker = std::thread([&]{
//                 this->worker();
//             });
//         }
//     }

//     template <class Task, class... Args>
//         requires Callable<Task, Args...>
//     bool appendTask(Task task, Args&&... args) {
//         if(m_stop){
//             return false;
//         }
//         auto fn = [task = std::move(task), ...args = std::forward<Args>(args)]() mutable {
//             task(std::forward<Args>(args)...);
//         };
//         {
//             std::lock_guard<std::mutex> lock(m_mtx);
//             ++m_activeTasks;
//             m_TaskList.push_back(std::move(fn));
//         }
//         m_cond.notify_one();
//         return true;
//     }

//     void wait() {
//         std::unique_lock<std::mutex> lock(m_mtx);
//         m_cond.wait(lock, [&]{
//             return m_activeTasks == 0 && m_TaskList.empty();
//         });
//     }


//     ~threadPool(){
//         m_stop = true;
//         m_cond.notify_all();
//         for(auto &worker : m_workers)
//         {
//             if(worker.joinable())
//             {
//                 worker.join();
//             }

//         }
//     }

// private:
//     threadPool(const threadPool&) = delete;
//     threadPool(threadPool&&) = delete;
//     threadPool& operator=(const threadPool&) = delete;
//     threadPool& operator=(threadPool&&) = delete;


//     void worker() {
//         std::function<void()> task;
//         while (true) {
//             {
//                 std::unique_lock<std::mutex> lock(m_mtx);
//                 m_cond.wait(lock,[&]{
//                     return !m_TaskList.empty() || m_stop;
//                 });
//                 if (m_TaskList.empty()) {
//                     if (m_stop) break;   // 队列空且要停止，才退出
//                     continue;
//                 }
//                 task = std::move(m_TaskList.front());
//                 m_TaskList.pop_front();
//             }
//             task();
//             {
//                 std::lock_guard<std::mutex> lock(m_mtx);
//                 if (m_activeTasks > 0) --m_activeTasks;
//                 if (m_activeTasks == 0) {
//                     m_cond.notify_all();
//                 }
//             }
//         }
//     }

//     const size_t m_numWorkers;
//     std::mutex m_mtx;
//     std::atomic<bool> m_stop{false};
//     std::condition_variable_any m_cond;
//     std::deque<std::function<void()>> m_TaskList;
//     std::vector<std::thread> m_workers;
//     size_t m_activeTasks{0};
// };

}  // namespace details

#endif