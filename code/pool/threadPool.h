#pragma once

#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <thread>
#include <future>
#include <iostream>
#include <assert.h>

class ThreadPool {
public:
    ThreadPool(const ThreadPool& o) = delete;
    ThreadPool(ThreadPool&& o) = delete;
    ThreadPool& operator=(const ThreadPool& o) = delete;
    ThreadPool& operator=(ThreadPool&& o) = delete;

    explicit ThreadPool(int ThreadCount = 8);
    ~ThreadPool();

    template<typename F, typename... Args>
    auto AddTask(F&& Task, Args&&... args) -> std::future<decltype(Task(args...))>;
private:
    void _workLoop();
private:
    bool isClose_ = false;
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;

    std::mutex mtx_;
    std::condition_variable cond_;
};

template<typename F, typename ...Args>
inline auto ThreadPool::AddTask(F&& Task, Args && ...args) -> std::future<decltype(Task(args ...))> {
    using ReturnType = decltype(Task(args...)); // 获取task返回值能行

    // 用packaged_task包裹任务函数，用于future获取任务函数的返回值。packaged_task只能移动不能拷贝。
    // 用shared_ptr是因为task是局部变量，而任务执行是在工作线程中，不用shared_ptr就被释放了。
    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
        std::bind(std::forward<F>(Task), std::forward<Args>(args)...)
    );

    std::future<ReturnType> future = task->get_future();
    {
        std::lock_guard<std::mutex> locker(mtx_);
        if (isClose_) {
            throw std::runtime_error("ThreadPool has closed!");
        }
        tasks_.emplace([task]() {(*task)();});
    }

    cond_.notify_one();     // 添加任务后，唤醒一个线程去执行。
    return future;
}
