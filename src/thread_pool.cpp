#include "thread_pool.h"

ThreadPool::ThreadPool(std::size_t threads_count) {
    for (std::size_t i = 0; i < threads_count; ++i) {
        workers_.emplace_back([this]() { WorkerLoop(); });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> lock(jobs_mutex_);
        stop_ = true;
    }
    cv_.notify_all();
    for (auto& t : workers_) {
        if (t.joinable()) {
            t.join();
        }
    }
}

void ThreadPool::Enqueue(std::function<void()> job) {
    {
        std::lock_guard<std::mutex> lock(jobs_mutex_);
        jobs_.push(std::move(job));
    }
    cv_.notify_one();
}

void ThreadPool::WorkerLoop() {
    while (true) {
        std::function<void()> job;
        {
            std::unique_lock<std::mutex> lock(jobs_mutex_);
            cv_.wait(lock, [this]() { return stop_ || !jobs_.empty(); });
            if (stop_ && jobs_.empty()) {
                return;
            }
            job = std::move(jobs_.front());
            jobs_.pop();
        }
        job();
    }
}
