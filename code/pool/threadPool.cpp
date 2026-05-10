#include "threadPool.h"

ThreadPool::ThreadPool(int ThreadCount) {
    assert(ThreadCount > 0);

    for (int i = 0; i < ThreadCount; ++i) {
        workers_.emplace_back(&ThreadPool::_workLoop, this);
    }
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard locker(mtx_);
        isClose_ = true;
    }
    cond_.notify_all();     // 唤醒所有阻塞的线程，处理剩余的任务，并结束

    for (auto&& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void ThreadPool::_workLoop() {
    while (true) {
        std::unique_lock locker(mtx_);
        cond_.wait(locker, [this]() {return isClose_ || !tasks_.empty();});     // 线程池关闭，或者任务队列非空

        if (isClose_ && tasks_.empty()) break;   // 线程池关闭了，所有任务执行完毕

        auto task = tasks_.front();
        tasks_.pop();
        locker.unlock();

        try {
            task();
        }
        catch (const std::exception& e) {

            std::cerr << e.what() << '\n';
        }
    }
}
