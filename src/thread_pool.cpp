#include "../header/thread_pool.h"

#include <algorithm>
#include <chrono>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__)
#include <immintrin.h>
static inline void cpuRelax() { _mm_pause(); }
#elif defined(__aarch64__) || defined(__arm__)
static inline void cpuRelax() { asm volatile("yield"); }
#else
static inline void cpuRelax() { std::this_thread::yield(); }
#endif

ThreadPool::ThreadPool(unsigned threadCount)
{
    if (threadCount == 0)
        threadCount = std::max(1u, std::thread::hardware_concurrency());
    for (unsigned i = 1; i < threadCount; ++i)
        workers_.emplace_back([this] { workerLoop(); });
}

ThreadPool::~ThreadPool()
{
    stop_.store(true);
    {
        std::lock_guard<std::mutex> lock(mutex_);
    }
    wake_.notify_all();
    for (auto& worker : workers_)
        worker.join();
}

void ThreadPool::parallelFor(int count, int grainSize, const std::function<void(int, int)>& fn)
{
    if (count <= 0)
        return;
    grainSize = std::max(1, grainSize);
    if (workers_.empty() || count <= grainSize)
    {
        fn(0, count);
        return;
    }

    job_ = &fn;
    count_ = count;
    grainSize_ = grainSize;
    next_.store(0, std::memory_order_relaxed);
    pending_.store(static_cast<int>(workers_.size()), std::memory_order_relaxed);
    generation_.fetch_add(1);

    if (sleepers_.load() > 0)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        wake_.notify_all();
    }

    runChunks();

    while (pending_.load(std::memory_order_acquire) != 0)
        cpuRelax();
    job_ = nullptr;
}

void ThreadPool::runChunks()
{
    for (;;)
    {
        const int begin = next_.fetch_add(grainSize_, std::memory_order_relaxed);
        if (begin >= count_)
            break;
        (*job_)(begin, std::min(begin + grainSize_, count_));
    }
}

void ThreadPool::workerLoop()
{
    using Clock = std::chrono::steady_clock;
    constexpr auto spinTime = std::chrono::microseconds(200);
    uint64_t seen = 0;

    for (;;)
    {
        const auto spinStart = Clock::now();
        int spins = 0;
        while (generation_.load() == seen && !stop_.load())
        {
            cpuRelax();
            if ((++spins & 63) != 0 || Clock::now() - spinStart < spinTime)
                continue;

            std::unique_lock<std::mutex> lock(mutex_);
            sleepers_.fetch_add(1);
            wake_.wait(lock, [&] { return generation_.load() != seen || stop_.load(); });
            sleepers_.fetch_sub(1);
        }

        if (stop_.load())
            return;

        seen = generation_.load();
        runChunks();
        pending_.fetch_sub(1, std::memory_order_acq_rel);
    }
}
