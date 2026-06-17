#pragma once

#include <new>
#include <atomic>
#include <cstddef>
#include <utility>

// ---------------------------------------------------------------------------
// Lock-Free Single-Producer Single-Consumer Queue
//
// Uses std::atomic with acquire-release memory ordering.
// Cached index copies (cached_read_index / cached_write_index) minimise
// cross-core cache-line traffic: the producer rarely reads the consumer's
// index and vice-versa, avoiding false sharing on the hot path.
//
// The two atomic indices are placed on separate cache lines via alignas so
// that the producer and consumer never invalidate each other's cache lines
// when updating their own index.
//
// API mirrors SpscQueueLocked so the benchmark driver works with both.
// ---------------------------------------------------------------------------

template <typename T, size_t capacity>
class SpscQueue
{
private:
    alignas(T) std::byte buffer[sizeof(T) * (capacity + 1)];
    
    alignas(std::hardware_destructive_interference_size) std::atomic<std::size_t> read_index{0};
    alignas(std::hardware_destructive_interference_size) std::atomic<std::size_t> write_index{0};

    size_t cached_read_index = 0;
    size_t cached_write_index = 0;

public:
    SpscQueue() = default;

    ~SpscQueue()
    {
        size_t start = read_index.load(std::memory_order_relaxed);
        size_t end = write_index.load(std::memory_order_relaxed);

        while (start != end)
        {
            reinterpret_cast<T*>(&buffer[start * sizeof(T)])->~T();
            start = (start + 1) % (capacity + 1);
        }
    }

    bool Enqueue(T&& in_item)
    {
        size_t write_idx = this->write_index.load(std::memory_order_relaxed);

        if (((write_idx + 1) % (capacity + 1)) == cached_read_index)
        {
            size_t read_idx = this->read_index.load(std::memory_order_acquire);
            cached_read_index = read_idx;
            if (((write_idx + 1) % (capacity + 1)) == read_idx) return false;
        }

        ::new (static_cast<void*>(&buffer[write_idx * sizeof(T)])) T(std::move(in_item));

        write_idx = (write_idx + 1) % (capacity + 1);
        this->write_index.store(write_idx, std::memory_order_release);
        return true;
    }

    bool Dequeue(T &out_item)
    {
        size_t read_idx = this->read_index.load(std::memory_order_relaxed);

        if (read_idx == cached_write_index)
        {
            size_t write_idx = this->write_index.load(std::memory_order_acquire);
            cached_write_index = write_idx;
            if (read_idx == write_idx) return false;
        }

        T* ptr = reinterpret_cast<T*>(&buffer[read_idx * sizeof(T)]);
        out_item = std::move(*ptr);
        ptr->~T();

        read_idx = (read_idx + 1) % (capacity + 1);
        this->read_index.store(read_idx, std::memory_order_release);
        return true;
    }
};