#pragma once
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

class ThreadPool
{
public:
    explicit ThreadPool(unsigned threadCount = 0);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    unsigned threadCount() const { return static_cast<unsigned>(workers_.size()) + 1; }

    void parallelFor(int count, int grainSize, const std::function<void(int, int)>& fn);

private:
    void workerLoop();
    void runChunks();

    std::vector<std::thread> workers_;

    const std::function<void(int, int)>* job_ = nullptr;
    int count_ = 0;
    int grainSize_ = 1;
    std::atomic<int> next_{0};
    std::atomic<int> pending_{0};
    std::atomic<uint64_t> generation_{0};
    std::atomic<bool> stop_{false};

    std::mutex mutex_;
    std::condition_variable wake_;
    std::atomic<int> sleepers_{0};
};
