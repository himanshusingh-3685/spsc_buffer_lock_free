cmake_minimum_required(VERSION 3.16)
project(spsc_queue_benchmark LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-interference-size)
endif()

find_package(Threads REQUIRED)

# --- benchmark_manual: the hand-written timing report, no external deps ---
add_executable(benchmark_manual main.cpp)
target_include_directories(benchmark_manual PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(benchmark_manual PRIVATE Threads::Threads)

# --- benchmark_gbench: Google Benchmark, fetched automatically ---
include(FetchContent)

set(BENCHMARK_ENABLE_TESTING  OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL  OFF CACHE BOOL "" FORCE)
set(BENCHMARK_DOWNLOAD_DEPENDENCIES ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG        v1.9.1
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(googlebenchmark)

add_executable(benchmark_gbench benchmark_gbench.cpp)
target_include_directories(benchmark_gbench PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(benchmark_gbench PRIVATE benchmark::benchmark_main Threads::Threads)
