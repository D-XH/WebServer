#include "threadPool.h"

ThreadPool::ThreadPool(int ThreadCount) : pool_(std::make_shared<Pool>()) {
    assert(ThreadCount > 0);
    for (int i = 0; i < ThreadCount; ++i) {
        std::thread([this] {
            std::unique_lock<std::mutex> locker(pool_->mtx);
            while (true) {
                if (!pool_->tasks.empty()) {
                    auto task = std::move(pool_->tasks.front());
                    pool_->tasks.pop();

                    locker.unlock();
                    task();
                    locker.lock();
                }
                else if (pool_->isClose) {
                    break;
                }
                else {
                    pool_->cond.wait(locker);
                }
            }
            }).detach();
    }
}

ThreadPool::~ThreadPool() {
    std::lock_guard<std::mutex> locker(pool_->mtx);
    if (pool_) {
        pool_->isClose = true;
        pool_->cond.notify_all();
    }
}
