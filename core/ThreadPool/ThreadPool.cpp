#include "ThreadPool.h"

ThreadPool::ThreadPool(int max_threads)
:_pool(std::make_shared<Pool>())
{
    assert(max_threads > 0);
    for(int i = 0; i < max_threads; ++i){
        thread([pool = _pool](){
            unique_lock<mutex> lock(pool->mtx);
            while(true){
                if(!pool->tasks.empty()){
                    auto task = pool->tasks.front();
                    pool->tasks.pop();
                    lock.unlock();
                    task();
                    lock.lock();
                }else if(pool->is_closed == true){
                    break;
                }else{
                    pool->cond.wait(lock);
                }
            }
        }).detach();
    }
}

ThreadPool::~ThreadPool()
{
    if(_pool){
        {
            lock_guard<mutex> lock(_pool->mtx);
            _pool->is_closed = true;
        }
        _pool->cond.notify_all();
    }
}
