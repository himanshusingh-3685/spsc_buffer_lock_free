#include <benchmark/benchmark.h>
#include <thread>

#include "spsc_queue_lock_free.tpp"
#include "spsc_queue_locked.tpp"

static constexpr size_t QUEUE_CAPACITY = 1024;

// Runs one full producer/consumer pass and returns when both threads are done.
// The producer runs on the calling thread; the consumer runs on a new thread.
template <typename Queue>
static void run_pass(Queue& q, int64_t item_count)
{
    std::thread consumer([&]() {
        int value = 0;
        for (int64_t i = 0; i < item_count; ++i)
            while (!q.Dequeue(value)) {}
    });

    for (int64_t i = 0; i < item_count; ++i) {
        int item = static_cast<int>(i);
        while (!q.Enqueue(std::move(item))) {}
    }

    consumer.join();
}

static void BM_LockFree(benchmark::State& state)
{
    const int64_t item_count = state.range(0);

    for (auto _ : state) {
        SpscQueue<int, QUEUE_CAPACITY> q;
        run_pass(q, item_count);
    }

    state.SetItemsProcessed(state.iterations() * item_count);
    state.SetBytesProcessed(state.iterations() * item_count * sizeof(int));
}

static void BM_Locked(benchmark::State& state)
{
    const int64_t item_count = state.range(0);

    for (auto _ : state) {
        SpscQueueLocked<int, QUEUE_CAPACITY> q;
        run_pass(q, item_count);
    }

    state.SetItemsProcessed(state.iterations() * item_count);
    state.SetBytesProcessed(state.iterations() * item_count * sizeof(int));
}

// Fixed 10 M items — matches the manual benchmark in main.cpp for direct comparison
BENCHMARK(BM_LockFree)->Arg(10'000'000)->Unit(benchmark::kMillisecond)->UseRealTime();
BENCHMARK(BM_Locked) ->Arg(10'000'000)->Unit(benchmark::kMillisecond)->UseRealTime();

// Sweep across item counts to see how throughput changes with load
BENCHMARK(BM_LockFree)->Arg(100'000)->Arg(1'000'000)->Arg(10'000'000)->Unit(benchmark::kMillisecond)->UseRealTime();
BENCHMARK(BM_Locked) ->Arg(100'000)->Arg(1'000'000)->Arg(10'000'000)->Unit(benchmark::kMillisecond)->UseRealTime();
