# SPSC Queue Benchmark — Lock-Free vs Mutex + Condition Variable

![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey.svg)

A C++17 micro-benchmark that compares two **Single-Producer Single-Consumer (SPSC)** ring-buffer implementations head-to-head, transferring **10 million elements** between a producer thread and a consumer thread.

| Implementation | File | Synchronisation strategy |
|---|---|---|
| **Lock-Free** | `spsc_queue_lock_free.tpp` | `std::atomic` acquire/release + cached index copies to eliminate false sharing |
| **Locked** | `spsc_queue_locked.tpp` | `std::mutex` + `std::condition_variable` — threads block rather than spin |

> **`.tpp` convention** — files with the `.tpp` extension are *template implementation* headers.  
> They are `#include`-d directly into `main.cpp` (not compiled separately), which is the standard  
> approach for header-only C++ template code that you want kept in a separate file from the class declaration.

---

## Project Structure

```
.
├── CMakeLists.txt              # Cross-platform build (CMake 3.16+)
├── LICENSE                     # MIT
├── README.md
├── main.cpp                    # Hand-written timing report (no external deps)
├── benchmark_gbench.cpp        # Google Benchmark suite
├── spsc_queue_lock_free.tpp    # Lock-free SPSC queue implementation
└── spsc_queue_locked.tpp       # Mutex + CV SPSC queue implementation
```

---

## Building

Requires a **C++17** compiler — GCC 9+, Clang 9+, or MSVC 2019+, and **CMake 3.16+**.

> **Google Benchmark is fetched automatically** via CMake `FetchContent` — no manual install needed.  
> The first build will download and compile it (~1–2 minutes); subsequent builds are instant.

### CMake (recommended)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

This produces **two executables** in `build/`:

| Executable | Description |
|---|---|
| `benchmark_manual` | Original hand-written report (`main.cpp`) — no external deps |
| `benchmark_gbench` | Google Benchmark suite with `--benchmark_*` flags and JSON export |

### Single-file manual build (no CMake, no Google Benchmark)

```bash
# GCC / MinGW / Clang
g++ -std=c++17 -O2 -o benchmark_manual main.cpp

# MSVC (Developer Command Prompt)
cl /std:c++17 /O2 /EHsc main.cpp /Fe:benchmark_manual.exe
```

---

## Running

### Manual benchmark

```bash
./build/benchmark_manual          # Linux / macOS
.\build\Release\benchmark_manual.exe  # Windows
```

### Google Benchmark

```bash
# Default run (all benchmarks, auto-tuned iterations)
./build/benchmark_gbench

# Run 5 repetitions and show mean/median/stddev
./build/benchmark_gbench --benchmark_repetitions=5 --benchmark_report_aggregates_only=true

# Export results to JSON (great for plotting / CI)
./build/benchmark_gbench --benchmark_format=json --benchmark_out=results.json

# Filter to only lock-free benchmarks
./build/benchmark_gbench --benchmark_filter=LockFree
```

### Google Benchmark columns explained

| Column | Meaning |
|---|---|
| `Time` | Real wall-clock time per benchmark iteration |
| `CPU` | CPU time consumed per iteration |
| `Iterations` | How many times Google Benchmark ran the loop to get stable numbers |
| `items_per_second` | Elements transferred per second (set via `SetItemsProcessed`) |
| `bytes_per_second` | Bytes moved per second (set via `SetBytesProcessed`) |

---

## Benchmark Results

> Run on **Intel Core i7 (12 × 2611 MHz), 10M elements, buffer capacity 1024, Windows 11, MSVC Release build.**

### benchmark_manual — hand-written timing report

```
  SPSC Queue Benchmark
  Items   : 10000000
  Capacity: 1024

  Running: Lock-Free  (SpscQueue) ...
  Done.  Elapsed: 76.266 ms

  Running: Locked     (SpscQueueLocked) ...
  Done.  Elapsed: 511.867 ms

================================================================
  SPSC QUEUE BENCHMARK REPORT
================================================================
  Metric                    Lock-Free         Mutex + CV
----------------------------------------------------------------
  Elements transferred      10000000          10000000
  Buffer capacity           1024              1024
  Elapsed time              76.266 ms         511.867 ms
  Throughput                131.12 M ops/s    19.54 M ops/s
----------------------------------------------------------------

  Speedup (Mutex+CV / Lock-Free): 6.71x
  >> Lock-Free is faster by 6.71x

================================================================
```

### benchmark_gbench — Google Benchmark (3 repetitions, mean/median/stddev)

#### Fixed load: 10 million items

| Benchmark | Mean time | Throughput (mean) | CV |
|---|---|---|---|
| `BM_LockFree/10000000` | **61.1 ms** | 164.19 M items/s | 7.08% |
| `BM_Locked/10000000` | **519 ms** | 19.28 M items/s | 2.20% |
| **Speedup** | **8.5×** | | |

#### Throughput sweep across item counts

| Items | Lock-Free (mean) | Locked (mean) | Lock-Free throughput | Locked throughput |
|---|---|---|---|---|
| 100,000 | 1.43 ms | 6.64 ms | 69.74 M items/s | 15.05 M items/s |
| 1,000,000 | 7.31 ms | 53.9 ms | 136.98 M items/s | 18.59 M items/s |
| 10,000,000 | 63.2 ms | 534 ms | 158.72 M items/s | 18.78 M items/s |

#### Raw Google Benchmark output

```
Run on (12 X 2611 MHz CPUs)
CPU Caches: L1 48 KiB (×6), L2 1280 KiB (×6), L3 12288 KiB (×1)

Benchmark                                  Time       CPU    Iterations
-----------------------------------------------------------------------
BM_LockFree/10000000/real_time_mean      61.1 ms   59.9 ms           3
BM_LockFree/10000000/real_time_median    60.4 ms   59.4 ms           3
BM_LockFree/10000000/real_time_stddev    4.32 ms   3.93 ms           3

BM_Locked/10000000/real_time_mean         519 ms    505 ms           3
BM_Locked/10000000/real_time_median       520 ms    500 ms           3
BM_Locked/10000000/real_time_stddev      11.4 ms   9.02 ms           3

BM_LockFree/100000/real_time_mean        1.43 ms  0.988 ms           3
BM_LockFree/1000000/real_time_mean       7.31 ms   6.44 ms           3
BM_LockFree/10000000/real_time_mean      63.2 ms   62.5 ms           3

BM_Locked/100000/real_time_mean          6.64 ms   5.21 ms           3
BM_Locked/1000000/real_time_mean         53.9 ms   52.1 ms           3
BM_Locked/10000000/real_time_mean         534 ms    516 ms           3
```

---

## How It Works

### Lock-Free Queue (`spsc_queue_lock_free.tpp`)

- Uses two `std::atomic<size_t>` indices — `write_index` (owned by producer) and `read_index` (owned by consumer).
- Each index is placed on its **own cache line** (`alignas(hardware_destructive_interference_size)`) to prevent false sharing.
- **Cached index copies** (`cached_read_index`, `cached_write_index`) mean each thread only crosses the cache-line boundary to read the *other* thread's index when it suspects the queue is full/empty — dramatically reducing inter-core traffic on the hot path.
- Memory ordering: `relaxed` load of own index → `acquire` load of other's index → placement-new/move → `release` store of own index.

### Locked Queue (`spsc_queue_locked.tpp`)

- Wraps the same ring-buffer layout in a `std::mutex`.
- Producer calls `cv_not_full.wait()` when full; consumer calls `cv_not_empty.wait()` when empty — threads yield the CPU instead of spinning.
- Simpler to reason about correctness but pays the cost of mutex acquisition and OS scheduler round-trips on every element.

---

## Why Is Lock-Free Faster?

With a **capacity-1024 buffer** and **10 M elements**, the producer fills the buffer faster than the consumer can drain it:

- The **lock-free** producer spins cheaply in user-space, checking a cached index copy without touching the other core's cache line most of the time.
- The **locked** producer acquires the mutex, finds the buffer full, calls `cv_not_full.wait()`, gets descheduled by the OS, gets woken back up, re-acquires the mutex — a round-trip that costs several microseconds per event.

Notably the **speedup shrinks as item count grows**: at 100K items (~4.6× gap) the queue rarely fills up so the locked version holds up better. At 10M items (~8.5× gap) the buffer saturates constantly and the OS context-switch cost compounds.

The gap would narrow further with a larger buffer capacity (fewer full-buffer events) or by pinning threads to specific CPU cores.

---

## License

[MIT](LICENSE) © 2026 Himanshu Singh
