#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

#include<assert.h>
#include<queue>
#include<thread>
#include<memory>
#include<mutex>
#include<condition_variable>
#include<functional>
using std::queue;
using std::thread;
using std::unique_lock;
using std::shared_ptr;
using std::lock_guard;
using std::function;
using std::mutex;
using std::condition_variable;

struct ThreadPool
{
public:
    explicit ThreadPool(int max_threads);
    ~ThreadPool();
    template<class F>
    void AddTask(F&& task);
private:
    struct Pool{
        bool is_closed;
        mutex mtx;
        condition_variable cond;
        queue<function<void()>> tasks;
    };
    shared_ptr<Pool> _pool;
};

template <class F>
inline void ThreadPool::AddTask(F &&task)
{
    lock_guard<mutex> lock(_pool->mtx);
    _pool->tasks.push(std::forward<F>(task));
}

#endif

