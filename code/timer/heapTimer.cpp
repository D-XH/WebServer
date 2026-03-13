#include "heapTimer.h"

HeapTimer::HeapTimer() {
    timers_.reserve(64);
}

HeapTimer::~HeapTimer() {
    timers_.clear();
    idToIdx_.clear();
}

void HeapTimer::Add(int Id, int Timeout, const TimeoutCallBack& cb) {
    assert(Id > 0);
    if (idToIdx_.count(Id)) {
        // 已有，则调整
        size_t idx = idToIdx_.find(Id)->second;
        timers_[idx].expire = Clock::now() + MS(Timeout);
        timers_[idx].cb = cb;
        if (!SiftDown_(idx, timers_.size())) {
            SiftUp_(idx);
        }
    }
    else {
        // 没有，添加
        TimeStamp expire = Clock::now() + MS(Timeout);

        idToIdx_[Id] = timers_.size();  // 插入前的size等于插入后的下标
        timers_.push_back({ Id, expire, cb });
        SiftUp_(idToIdx_[Id]);
    }

}

void HeapTimer::Adjust(int Id, int Timeout) {
    assert(!timers_.empty() && idToIdx_.count(Id));
    timers_[idToIdx_[Id]].expire = Clock::now() + MS(Timeout);
    if (!SiftDown_(idToIdx_[Id], timers_.size())) {
        SiftUp_(idToIdx_[Id]);
    }
}

void HeapTimer::DoWork(int Id) {
    if (timers_.empty() || idToIdx_.count(Id) == 0) {
        return;
    }

    timers_[idToIdx_[Id]].cb();
    Del_(Id);
}

void HeapTimer::Tick() {
    TimeStamp now = Clock::now();
    while (!timers_.empty()) {
        const auto& node = timers_.front();

        // if (node.expire > now) { break; }
        if (std::chrono::duration_cast<MS>(node.expire - now).count() > 0) { break; }

        node.cb();
        Del_(node.id);
    }
}

int HeapTimer::GetNextTick() {
    Tick();
    int nextTick = -1;
    if (!timers_.empty()) {
        nextTick = std::chrono::duration_cast<MS>(timers_.front().expire - Clock::now()).count();
        if (nextTick < 0) { nextTick = 0; }
    }
    return nextTick;
}

void HeapTimer::SiftUp_(size_t Idx) {
    assert(Idx >= 0 && Idx < timers_.size());
    size_t pIdx = (Idx - 1) / 2;
    while (pIdx >= 0) {
        if (timers_[pIdx] > timers_[Idx]) {
            SwapNode_(pIdx, Idx);
            Idx = pIdx;
            pIdx = (Idx - 1) / 2;
        }
        else {
            break;
        }
    }
}

bool HeapTimer::SiftDown_(size_t Idx, size_t n) {
    assert(n >= 0 && n <= timers_.size());
    assert(Idx >= 0 && Idx < timers_.size());

    size_t rawIdx = Idx;
    size_t cIdx = Idx * 2 + 1;
    while (cIdx < n) {
        if (cIdx + 1 < n && timers_[cIdx] > timers_[cIdx + 1]) {
            ++cIdx;
        }
        if (timers_[Idx] > timers_[cIdx]) {
            SwapNode_(Idx, cIdx);
            Idx = cIdx;
            cIdx = Idx * 2 + 1;
        }
        else {
            break;
        }
    }
    return rawIdx != Idx;
}

void HeapTimer::SwapNode_(size_t Idx1, size_t Idx2) {
    assert(Idx1 >= 0 && Idx1 < timers_.size());
    assert(Idx2 >= 0 && Idx2 < timers_.size());
    if (Idx1 == Idx2) return;

    std::swap(timers_[Idx1], timers_[Idx2]);
    idToIdx_[timers_[Idx1].id] = Idx1;
    idToIdx_[timers_[Idx2].id] = Idx2;
}

void HeapTimer::Del_(int Id) {
    if (idToIdx_.count(Id)) {
        size_t idx = idToIdx_.find(Id)->second;

        // 从timers_中删除
        SwapNode_(idx, timers_.size() - 1);
        timers_.pop_back();

        // 从idToIdx_中删除
        idToIdx_.erase(Id);
        if (!timers_.empty()) SiftDown_(idx, timers_.size());
    }
}
