#pragma once

#include <vector>
#include <thread>
#include <mutex>
#include <utility>

template<typename Func>
void ParallelWithLimit(const std::vector<std::pair<std::wstring, std::wstring>>& paths, size_t max_threads, Func&& fn)
{
    std::mutex mutex;
    size_t next = 0;
    std::vector<std::thread> workers;

    for (size_t t = 0; t < max_threads; ++t) {
        workers.emplace_back([&]() {
            while (true) {
                std::pair<std::wstring, std::wstring> item;
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    if (next >= paths.size()) return;
                    item = paths[next++];
                }
                fn(item);
            }
            });
    }

    for (auto& th : workers) {
        if (th.joinable()) th.join();
    }
}

size_t GetOptimalThreadCount() 
{
    size_t thread_count = std::thread::hardware_concurrency();
    return thread_count == 0 ? 4 : thread_count;
}