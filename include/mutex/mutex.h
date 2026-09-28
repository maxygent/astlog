#ifndef ASTLOG_MUTEX_H
#define ASTLOG_MUTEX_H

#include <pthread.h>

namespace details {

struct mutex {
    mutex() {
        pthread_mutex_init(&m_mtx, nullptr);
    }

    mutex(const mutex&) = delete;
    mutex& operator=(const mutex&) = delete;
    mutex(mutex&&) = delete;
    mutex& operator=(mutex&&) = delete;

    void lock() {
        pthread_mutex_lock(&m_mtx);
    }

    void unlock() {
        pthread_mutex_unlock(&m_mtx);
    }

    bool try_lock() {
        return pthread_mutex_trylock(&m_mtx) == 0;
    }

    pthread_mutex_t* native_handle() {
        return &m_mtx;
    }

    ~mutex() {
        pthread_mutex_destroy(&m_mtx);
    }

private:
    pthread_mutex_t m_mtx;
};

}  // namespace details

#endif