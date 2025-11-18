#pragma once
#include <vector>
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>

class ThreadPool {
public:
    explicit ThreadPool(std::size_t threads_count);
    ~ThreadPool();

    void Enqueue(std::function<void()> job);

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> jobs_;
    std::mutex jobs_mutex_;
    std::condition_variable cv_;
    bool stop_ = false;

    void WorkerLoop();
};
