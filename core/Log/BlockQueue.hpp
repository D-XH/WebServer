#ifndef __BLOCK_QUEUE_H__
#define __BLOCK_QUEUE_H__

#include<memory>
#include<deque>
#include<mutex>
#include<condition_variable>
#include"assert.h"

using std::deque;
using std::mutex;
using std::condition_variable;
using std::lock_guard;
using std::unique_lock;

template<class T>
struct BlockQueue
{
public:
    explicit BlockQueue(size_t que_size = 1000)
    :_capacity(que_size)
    {
        assert(_capacity > 0);
        printf("%ld\n", _que.size());
        _isClosed = false;
    }
    ~BlockQueue(){
        Close();
    }

    void Push(const T& elem){
        unique_lock<mutex> lock(_mtx);
        _producer_cond.wait(lock, [this]{return _que.size() < _capacity;});
        _que.push_back(elem);
        lock.unlock();
        _consumer_cond.notify_one();
    }

    void Pop(T* front){
        unique_lock<mutex> lock(_mtx);
        while(_que.empty()){
            _consumer_cond.wait(lock);
            if(_isClosed){
                return;
            }
        }
        *front = _que.front();
        _que.pop_front();
        lock.unlock();
        _producer_cond.notify_one();
    }

    T& Front(){
        lock_guard<mutex> lock(_mtx);
        assert(!Empty());
        return _que.front();
    }

    void Flush(){
        _consumer_cond.notify_one();
    }

    bool IsClosed(){
        lock_guard<mutex> lock(_mtx);
        return _isClosed;
    }

    void Close(){
        if(!_isClosed){
            {
                lock_guard<mutex> lock(_mtx);
                _isClosed = true;
            }
            _producer_cond.notify_all();
            _consumer_cond.notify_all();
        }
    }

    size_t Size(){
        lock_guard<mutex> lock(_mtx);
        return _que.size();
    }

    size_t Capacity(){
        lock_guard<mutex> lock(_mtx);
        return _capacity;
    }

    bool Empty(){
        lock_guard<mutex> lock(_mtx);
        return _que.empty();
    }

    bool Full(){
        lock_guard<mutex> lock(_mtx);
        return _que.size() >= _capacity;
    }

    void Clear(){
        lock_guard<mutex> lock(_mtx);
        _que.clear();
    }

    void Resize(size_t new_cap){
        lock_guard<mutex> lock(_mtx);
        _que.resize(new_cap);
        _capacity = new_cap;
    }

private:
    deque<T> _que;
    size_t _capacity;
    bool _isClosed;
private:
    mutex _mtx;
    condition_variable _producer_cond;
    condition_variable _consumer_cond;
};

#endif