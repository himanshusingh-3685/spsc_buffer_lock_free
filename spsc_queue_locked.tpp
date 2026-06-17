#pragma once

#include <mutex>
#include <condition_variable>
#include <cstddef>
#include <utility>
#include <new>

// Locked SPSC queue using std::mutex + std::condition_variable.
// API mirrors SpscQueue (lock-free) so both can be driven by the same benchmark.
template <typename T, size_t capacity>
class SpscQueueLocked
{
private:
    alignas(T) std::byte buffer[sizeof(T) * (capacity + 1)];

    size_t read_index  = 0;
    size_t write_index = 0;

    std::mutex              mtx;
    std::condition_variable cv_not_full;    // signalled when a slot is freed
    std::condition_variable cv_not_empty;   // signalled when an item is pushed

public:
    SpscQueueLocked() = default;

    SpscQueueLocked(const SpscQueueLocked&)            = delete;
    SpscQueueLocked& operator=(const SpscQueueLocked&) = delete;

    ~SpscQueueLocked()
    {
        while (read_index != write_index)
        {
            reinterpret_cast<T*>(&buffer[read_index * sizeof(T)])->~T();
            read_index = (read_index + 1) % (capacity + 1);
        }
    }

    bool Enqueue(T&& in_item)
    {
        std::unique_lock<std::mutex> lock(mtx);

        // Guard against spurious wakeups — re-check the condition each time.
        while ((write_index + 1) % (capacity + 1) == read_index)
            cv_not_full.wait(lock);

        ::new (static_cast<void*>(&buffer[write_index * sizeof(T)])) T(std::move(in_item));
        write_index = (write_index + 1) % (capacity + 1);

        lock.unlock();
        cv_not_empty.notify_one();
        return true;
    }

    bool Dequeue(T& out_item)
    {
        std::unique_lock<std::mutex> lock(mtx);

        // Guard against spurious wakeups — re-check the condition each time.
        while (read_index == write_index)
            cv_not_empty.wait(lock);

        T* ptr   = reinterpret_cast<T*>(&buffer[read_index * sizeof(T)]);
        out_item = std::move(*ptr);
        ptr->~T();
        read_index = (read_index + 1) % (capacity + 1);

        lock.unlock();
        cv_not_full.notify_one();
        return true;
    }
};
