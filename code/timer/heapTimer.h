#pragma once

#include <functional>
#include <chrono>
#include <vector>
#include <unordered_map>

#include <assert.h>

typedef std::function<void()> TimeoutCallBack;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;


struct TimerNode
{
    int id;             // timer Id
    TimeStamp expire;   // 过期时间
    TimeoutCallBack cb; // 回调

    bool operator<(const TimerNode& o) {
        return expire < o.expire;
    }

    bool operator>(const TimerNode& o) {
        return expire > o.expire;
    }
};

class HeapTimer {
public:
    HeapTimer();
    ~HeapTimer();

    void Add(int Id, int Timeout, const TimeoutCallBack& cb);
    void Adjust(int Id, int Timeout);

    void DoWork(int Id);
    void Tick();

    int GetNextTick();
private:
    void SiftUp_(size_t Idx);
    bool SiftDown_(size_t Idx, size_t n);
    void SwapNode_(size_t Idx1, size_t Idx2);

    void Del_(int Id);
private:
    std::vector<TimerNode> timers_;
    std::unordered_map<int, size_t> idToIdx_;
};
