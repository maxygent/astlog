#ifndef ASTLOG_LOCK_HEAD_H
#define ASTLOG_LOCK_HEAD_H

#include <concepts>

#include <pthread.h>

template <class Lock>
concept Lockable = requires(Lock lck) {
    { lck.lock() } -> std::same_as<void>;
    { lck.unlock() } -> std::same_as<void>;
    { lck.try_lock() } -> std::convertible_to<bool>;
};

template <class Lock>
concept RWLockable = requires(Lock lck) {
    { lck.rdLock() } -> std::same_as<void>;
    { lck.wrLock() } -> std::same_as<void>;
    { lck.unlock() } -> std::same_as<void>;
};

#endif