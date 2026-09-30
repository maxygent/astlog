#ifndef ASTLOG_SPSC_RING_H
#define ASTLOG_SPSC_RING_H

#include <atomic>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include <cassert>

namespace astlog::details {

// Single-producer single-consumer lock-free ring buffer.
// Capacity is rounded up to next power-of-two. Usable size = capacity - 1.
// Producer: call try_push / try_emplace
// Consumer: call try_pop

template <typename T>
class SPSCQueue {
public:
    explicit SPSCQueue(size_t capacity = 1024) {
        assert(capacity > 0);
        // We need one slot free to distinguish full/empty, so allocate at least capacity+1
        size_t cap = 1;
        while (cap < capacity + 1) cap <<= 1;
        capacity_ = cap;
        mask_ = capacity_ - 1;
        // allocate raw storage (operator new returns memory suitably aligned for T)
        storage_ = std::unique_ptr<std::byte, void(*)(void*)>(
            static_cast<std::byte*>(operator new(sizeof(T) * capacity_)),
            &SPSCQueue::storage_deleter);
        head_.store(0, std::memory_order_relaxed);
        tail_.store(0, std::memory_order_relaxed);
    }

    ~SPSCQueue() {
        // destroy remaining elements
        size_t h = head_.load(std::memory_order_relaxed);
        size_t t = tail_.load(std::memory_order_relaxed);
        while (h != t) {
            T* ptr = ptr_at(h);
            ptr->~T();
            h = (h + 1) & mask_;
        }
        // storage_ will be released by unique_ptr deleter
        storage_.reset();
    }

    SPSCQueue(const SPSCQueue&) = delete;
    SPSCQueue& operator=(const SPSCQueue&) = delete;

    // Try to push a copy. Returns false if queue is full.
    bool try_push(const T& v) {
        const size_t tail = tail_.load(std::memory_order_relaxed);
        const size_t next = (tail + 1) & mask_;
        const size_t head = head_.load(std::memory_order_acquire);
        if (next == head) return false; // full
        T* slot = ptr_at(tail);
        ::new (static_cast<void*>(slot)) T(v);
        tail_.store(next, std::memory_order_release);
        return true;
    }

    // Try to push a move. Returns false if full.
    bool try_push(T&& v) {
        const size_t tail = tail_.load(std::memory_order_relaxed);
        const size_t next = (tail + 1) & mask_;
        const size_t head = head_.load(std::memory_order_acquire);
        if (next == head) return false; // full
        T* slot = ptr_at(tail);
        ::new (static_cast<void*>(slot)) T(std::move(v));
        tail_.store(next, std::memory_order_release);
        return true;
    }

    // Try to emplace. Returns false if full.
    template <typename... Args>
    bool try_emplace(Args&&... args) {
        const size_t tail = tail_.load(std::memory_order_relaxed);
        const size_t next = (tail + 1) & mask_;
        const size_t head = head_.load(std::memory_order_acquire);
        if (next == head) return false; // full
        T* slot = ptr_at(tail);
        ::new (static_cast<void*>(slot)) T(std::forward<Args>(args)...);
        tail_.store(next, std::memory_order_release);
        return true;
    }

    // Try to pop into out. Returns false if empty.
    bool try_pop(T& out) {
        const size_t head = head_.load(std::memory_order_relaxed);
        const size_t tail = tail_.load(std::memory_order_acquire);
        if (head == tail) return false; // empty
        T* slot = ptr_at(head);
        out = std::move(*slot);
        slot->~T();
        const size_t next = (head + 1) & mask_;
        head_.store(next, std::memory_order_release);
        return true;
    }

    // Peek into front without removing (copy). Returns false if empty.
    bool peek(T& out) const {
        const size_t head = head_.load(std::memory_order_acquire);
        const size_t tail = tail_.load(std::memory_order_acquire);
        if (head == tail) return false;
        T* slot = const_cast<SPSCQueue*>(this)->ptr_at(head);
        out = *slot;
        return true;
    }

    // Returns true if queue empty
    bool empty() const noexcept {
        return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
    }

    // Returns approximate size. For SPSC this is consistent but may race with concurrent push/pop.
    size_t size() const noexcept {
        size_t head = head_.load(std::memory_order_acquire);
        size_t tail = tail_.load(std::memory_order_acquire);
        if (tail >= head) return tail - head;
        return (capacity_ - head) + tail;
    }

    // Returns usable capacity (max elements that can be stored)
    size_t capacity() const noexcept { return capacity_ - 1; }

    void clear() {
        T tmp;
        while (try_pop(tmp)) {}
    }

private:
    static void storage_deleter(void* p) { operator delete(p); }

    T* ptr_at(size_t idx) {
        return reinterpret_cast<T*>(storage_.get() + idx * sizeof(T));
    }

    std::unique_ptr<std::byte, void(*)(void*)> storage_;
    size_t capacity_;
    size_t mask_;
    alignas(64) std::atomic<size_t> head_{}; // consumer read index
    alignas(64) std::atomic<size_t> tail_{}; // producer write index
};

} // namespace astlog::details

#endif // ASTLOG_SPSC_RING_H
