#include <cstddef>
#include "membuf.h"
#include "sink/sink.h"
#include "thread_pool.h"

namespace details {

struct threadPool::impl {
    impl(size_t num) {
        m_workList.resize(num ? num : 1U);
        for (auto &worker : m_workList) {
            worker = std::thread([this] { this->worker(); });
        }
    }

    ~impl() {
        {
            std::lock_guard<std::mutex> lock(m_mtx);
            m_stop = true;
        }
        m_cond.notify_all();
        for (auto &worker : m_workList) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    void postmsg(const async_msg &msg) {
        {
            std::lock_guard<std::mutex> lock(m_mtx);
            if (m_stop) {
                return;
            }
            m_buffer.push(msg);
        }
        m_cond.notify_one();
    }

private:
    void worker() {
        while (true) {
            async_msg msg{};
            {
                std::unique_lock<std::mutex> lock(m_mtx);
                m_cond.wait(lock, [this] {
                    return m_stop || !m_buffer.empty();
                });
                if (m_stop && m_buffer.empty()) {
                    return;
                }
                async_msg *queued = m_buffer.front();
                if (queued == nullptr) {
                    continue;
                }
                msg = *queued;
                m_buffer.pop();
            }
            if (msg.m_sinker) {
                msg.m_sinker->log(msg.m_msg);
            }
        }
    }

    std::mutex m_mtx;
    std::condition_variable m_cond;
    std::vector<std::thread> m_workList;
    DataBuf<async_msg> m_buffer;
    bool m_stop{false};
};

threadPool::threadPool(size_t num) : m_impl(std::make_shared<impl>(num)) {}

void threadPool::postmsg(sink::sinkPtr sinker,const logmsg& msg) {
    async_msg asmsg{sinker, msg};
    m_impl->postmsg(asmsg);
}

}  // namespace details