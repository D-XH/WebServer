#pragma once

#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <thread>
#include <assert.h>

class ThreadPool {
public:
    ThreadPool() = default;
    explicit ThreadPool(int ThreadCount = 8);
    ~ThreadPool();

    template<typename T>
    void AddTask(T&& Task);
private:
    struct Pool
    {
        bool isClose;
        std::queue<std::function<void()>> tasks;

        std::mutex mtx;
        std::condition_variable cond;
    };

    std::shared_ptr<Pool> pool_;
};

template<typename T>
inline void ThreadPool::AddTask(T&& Task) {
    std::unique_lock<std::mutex> locker(pool_->mtx);
    if (pool_->isClose) {
        return;
    }

    pool_->tasks.emplace(std::forward<T>(Task));
    pool_->cond.notify_one();
}
