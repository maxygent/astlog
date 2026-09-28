#ifndef ASTLOG_THREAD_POOL_H
#define ASTLOG_THREAD_POOL_H

#include <array>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <atomic>
#include <utility>
#include <thread>
#include <mutex>


namespace details {

template <class Func, class... Args>
concept Callable = requires(Func fn, Args&&... args) {
    fn(std::forward<Args>(args)...);
};

template <size_t NumWorkers>
class threadPool {
public:
    static threadPool& getInstance(){
        static threadPool pool;
        return pool;
    }

    threadPool() {
        for (auto& worker : m_workers) {
            worker = std::thread([&]{
                this->worker();
            });
        }
    }


    template <class Task, class... Args>
        requires Callable<Task, Args...>
    bool appendTask(Task task, Args&&... args) {
        if(m_stop){
            return false;
        }
        auto fn = [task = std::move(task), ...args = std::forward<Args>(args)]() mutable {
            task(std::forward<Args>(args)...);
        };
        {
            std::lock_guard<std::mutex> lock(m_mtx);
            // m_cond.wait(m_mtx,[&]{
            //     return !this->m_TaskList.empty();
            // });
            m_TaskList.push_back(std::move(fn));
            return true;
        }
        m_cond.notify_one();
    }


    ~threadPool(){
        m_stop = true;
        m_cond.notify_all();
        for(auto &worker : m_workers)
        {
            if(worker.joinable())
            {
                worker.join();
            }

        }
    }

private:


    threadPool(const threadPool&) = delete;
    threadPool(threadPool&&) = delete;
    threadPool& operator=(const threadPool&) = delete;
    threadPool& operator=(threadPool&&) = delete;






    void worker() {
        std::function<void()> task;
        while (true) {
            {
                std::unique_lock<std::mutex> lock(m_mtx);
                m_cond.wait(lock,[&]{
                    return !m_TaskList.empty() || m_stop;
                });
                if(m_stop){
                    break;
                }
                task = std::move(m_TaskList.front());
                m_TaskList.pop_front();
            }
            task();
        }
    }

    std::mutex m_mtx;
    std::atomic<bool> m_stop;
    std::condition_variable_any m_cond;
    std::deque<std::function<void()>> m_TaskList;
    std::array<std::thread, NumWorkers> m_workers;
};

}  // namespace details

#endif