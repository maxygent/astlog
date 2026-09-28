#ifndef ASTLOG_RW_MUTEX_H
#define ASTLOG_RW_MUTEX_H

#include <pthread.h>
#include <stdexcept>

#include "lock_head.h"

namespace details {

class rwMutex {
public:
    rwMutex() {
        if (pthread_rwlock_init(&m_rwLock, nullptr) != 0) {
            throw std::runtime_error("pthread_rwlock_init failed");
        }
    }

    rwMutex(const rwMutex&) = delete;
    rwMutex& operator=(const rwMutex&) = delete;
    rwMutex(rwMutex&&) = delete;
    rwMutex& operator=(rwMutex&&) = delete;

    void rdLock() {
        pthread_rwlock_rdlock(&m_rwLock);
    }

    void wrLock() {
        pthread_rwlock_wrlock(&m_rwLock);
    }

    void unlock() {
        pthread_rwlock_unlock(&m_rwLock);
    }

    ~rwMutex() {
        pthread_rwlock_destroy(&m_rwLock);
    }

private:
    pthread_rwlock_t m_rwLock;
};

template <RWLockable Lock>
struct readGuard {
    explicit readGuard(Lock& mtx) : m_mtx(mtx) {
        mtx.rdLock();
    }

    ~readGuard() {
        m_mtx.unlock();
    }

private:
    Lock& m_mtx;
};

template <RWLockable Lock>
struct writeGuard {
    explicit writeGuard(Lock& mtx) : m_mtx(mtx) {
        mtx.wrLock();
    }

    ~writeGuard() {
        m_mtx.unlock();
    }

private:
    Lock& m_mtx;
};

}  // namespace details

#endif