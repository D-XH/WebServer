#pragma once

#include <vector>

#include <sys/epoll.h>

#include "../log/log.h"

class Epoller {
public:
    Epoller(int MaxEvent = 1024);
    ~Epoller();

    void AddFd(int Fd, uint32_t EventMode);
    void ModFd(int Fd, uint32_t EventMode);
    void DelFd(int Fd);

    int Wait(int Timeout = -1);
    int GetEventFd(size_t idx) const;
    uint32_t GetEventMode(size_t idx) const;

private:
    int epFd_;
    std::vector<epoll_event> events_;
};