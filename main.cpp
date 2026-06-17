#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <string>
#include <sstream>

#include "spsc_queue_lock_free.tpp"
#include "spsc_queue_locked.tpp"

static constexpr int    ITEM_COUNT = 10'000'000;
static constexpr size_t CAPACITY   = 1024;

static void print_row(const char* label, const std::string& lf, const std::string& lk)
{
    std::cout << std::left
              << std::setw(28) << label
              << std::setw(18) << lf
              << std::setw(18) << lk
              << '\n';
}

static std::string format_throughput(double ops_per_sec)
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);

    if (ops_per_sec >= 1e9)       ss << ops_per_sec / 1e9 << " B ops/s";
    else if (ops_per_sec >= 1e6)  ss << ops_per_sec / 1e6 << " M ops/s";
    else if (ops_per_sec >= 1e3)  ss << ops_per_sec / 1e3 << " K ops/s";
    else                          ss << ops_per_sec        << " ops/s";

    return ss.str();
}

template <typename Queue>
double run_benchmark(const char* label, Queue& q)
{
    using clock = std::chrono::high_resolution_clock;

    std::cout << "\n  Running: " << label << " ...\n";
    auto t_start = clock::now();

    std::thread consumer([&]() {
        int value = 0;
        for (int i = 0; i < ITEM_COUNT; ++i)
            while (!q.Dequeue(value)) {}
    });

    std::thread producer([&]() {
        for (int i = 0; i < ITEM_COUNT; ++i) {
            int item = i;
            while (!q.Enqueue(std::move(item))) {}
        }
    });

    producer.join();
    consumer.join();

    auto t_end = clock::now();
    double elapsed_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();

    std::cout << "  Done.  Elapsed: " << std::fixed << std::setprecision(3) << elapsed_ms << " ms\n";
    return elapsed_ms;
}

static void print_report(double lf_ms, double lk_ms)
{
    double lf_throughput = ITEM_COUNT / (lf_ms / 1000.0);
    double lk_throughput = ITEM_COUNT / (lk_ms / 1000.0);
    double speedup       = lk_ms / lf_ms;

    auto ms_str = [](double ms) {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(3) << ms << " ms";
        return ss.str();
    };

    std::string sep(64, '=');
    std::string sep2(64, '-');

    std::cout << "\n\n" << sep << "\n";
    std::cout << "  SPSC QUEUE BENCHMARK REPORT\n";
    std::cout << sep << "\n";

    print_row("  Metric",             "Lock-Free",                    "Mutex + CV");
    std::cout << sep2 << "\n";
    print_row("  Elements transferred", std::to_string(ITEM_COUNT),   std::to_string(ITEM_COUNT));
    print_row("  Buffer capacity",      std::to_string(CAPACITY),      std::to_string(CAPACITY));
    print_row("  Elapsed time",         ms_str(lf_ms),                 ms_str(lk_ms));
    print_row("  Throughput",           format_throughput(lf_throughput), format_throughput(lk_throughput));
    std::cout << sep2 << "\n";

    std::cout << "\n  Speedup (Mutex+CV / Lock-Free): "
              << std::fixed << std::setprecision(2) << speedup << "x\n";

    if (speedup > 1.0)
        std::cout << "  >> Lock-Free is faster by " << speedup << "x\n";
    else if (speedup < 1.0)
        std::cout << "  >> Mutex+CV is faster by " << (1.0 / speedup) << "x  (consider re-running)\n";
    else
        std::cout << "  >> Both performed equally.\n";

    std::cout << "\n" << sep << "\n\n";
}

int main()
{
    std::cout << "\n  SPSC Queue Benchmark\n"
              << "  Items   : " << ITEM_COUNT << "\n"
              << "  Capacity: " << CAPACITY   << "\n";

    SpscQueue<int, CAPACITY> lf_queue;
    double lf_ms = run_benchmark("Lock-Free  (SpscQueue)", lf_queue);

    SpscQueueLocked<int, CAPACITY> lk_queue;
    double lk_ms = run_benchmark("Locked     (SpscQueueLocked)", lk_queue);

    print_report(lf_ms, lk_ms);

    return 0;
}