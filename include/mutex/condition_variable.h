#ifndef ASTLOG_CONDITION_VARIABLE_H
#define ASTLOG_CONDITION_VARIABLE_H

#include <pthread.h>

#include "lock_head.h"

namespace details {

struct conditionVariable {
    conditionVariable() {
        pthread_cond_init(&m_cond, nullptr);
    }

    ~conditionVariable() {
        pthread_cond_destroy(&m_cond);
    }

    void wait(pthread_mutex_t& mtx) {
        pthread_cond_wait(&m_cond, &mtx);
    }

    template <class Pred>
    void wait(pthread_mutex_t& mtx, Pred&& pred) {
        while (!pred()) {
            pthread_cond_wait(&m_cond, &mtx);
        }
    }

    void notify_one() {
        pthread_cond_signal(&m_cond);
    }

    void notify_all() {
        pthread_cond_broadcast(&m_cond);
    }

private:
    pthread_cond_t m_cond;
};

}  // namespace details

#endif