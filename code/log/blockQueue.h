#pragma once

#include <unistd.h>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <assert.h>

template<typename T>
class BlockQueue {
public:
    BlockQueue(size_t MaxSize = 1024);
    ~BlockQueue();
    void Close();
    void Flush();

    bool Empty();
    bool Full();
    void Clear();
    size_t Size();
    size_t Capacity();

    void PushBack(const T& Item);
    void PushFront(const T& Item);
    bool Pop(T& Item);
    bool Pop(T& Item, int TimeOut);

    T Front();
    T Back();


private:
    bool isClose_;

    std::deque<T> deq_;     // 用作队列
    size_t capacity_;       // 容量

    std::mutex mtx_;
    std::condition_variable condConsumer_;
    std::condition_variable condProducer_;
};

template<typename T>
inline BlockQueue<T>::BlockQueue(size_t MaxSize) : capacity_(MaxSize) {
    assert(MaxSize > 0);
    isClose_ = false;
}

template<typename T>
inline BlockQueue<T>::~BlockQueue() {
    Close();
}

template<typename T>
inline bool BlockQueue<T>::Empty() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.empty();
}

template<typename T>
inline bool BlockQueue<T>::Full() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.size() >= capacity_;
}

template<typename T>
inline void BlockQueue<T>::Clear() {
    std::lock_guard<std::mutex> locker(mtx_);
    deq_.clear();
}

template<typename T>
inline size_t BlockQueue<T>::Size() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.size();
}

template<typename T>
inline size_t BlockQueue<T>::Capacity() {
    std::lock_guard<std::mutex> locker(mtx_);
    return capacity_;
}

template<typename T>
inline void BlockQueue<T>::Close() {
    std::lock_guard<std::mutex> locker(mtx_);
    isClose_ = true;
    condConsumer_.notify_all();
    condProducer_.notify_all();
}

template<typename T>
inline void BlockQueue<T>::Flush() {
    condConsumer_.notify_one();
}

template<typename T>
inline void BlockQueue<T>::PushBack(const T& Item) {
    std::unique_lock<std::mutex> locker(mtx_);
    while (deq_.size() >= capacity_) {
        // 队列满了，等待
        condProducer_.wait(locker);
    }
    deq_.push_back(Item);
    condConsumer_.notify_one();
}

template<typename T>
inline void BlockQueue<T>::PushFront(const T& Item) {
    std::unique_lock<std::mutex> locker(mtx_);
    while (deq_.size() >= capacity_) {
        condProducer_.wait(locker);
    }
    deq_.push_front(Item);
    condConsumer_.notify_one();
}

template<typename T>
inline bool BlockQueue<T>::Pop(T& Item) {
    std::unique_lock<std::mutex> locker(mtx_);
    while (deq_.empty()) {
        condConsumer_.wait(locker);
        if (isClose_) {
            return false;
        }
    }
    Item = deq_.front();
    deq_.pop_front();
    condProducer_.notify_one();
    return true;
}

template<typename T>
inline bool BlockQueue<T>::Pop(T& Item, int TimeOut) {
    std::unique_lock<std::mutex> locker(mtx_);
    while (deq_.empty()) {
        if (condConsumer_.wait_for(locker, std::chrono::seconds(TimeOut)) == std::cv_status::timeout) {
            return false;
        }
        if (isClose_) {
            return false;
        }
    }
    Item = deq_.front();
    deq_.pop_front();
    condProducer_.notify_one();
    return true;
}

template<typename T>
inline T BlockQueue<T>::Front() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.front();
}

template<typename T>
inline T BlockQueue<T>::Back() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.back();
}
