#include "Epoller.h"

Epoller::Epoller(size_t max_events)
:_epoll_fd(epoll_create1(0)) ,_events(max_events)
{
    assert((_epoll_fd != -1) && (_events.size() == max_events));
}

Epoller::~Epoller()
{
    close(_epoll_fd);
}

bool Epoller::AddFd(int fd, uint32_t event)
{
    if(fd < 0){
        return false;
    }
    epoll_event evt = {0};
    evt.data.fd = fd;
    evt.events = event;
    if(epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, fd, &evt) < 0){
        // log_info
        return false;
    }
    return true;
}

bool Epoller::ModFd(int fd, uint32_t event)
{
    if(fd < 0){
        return false;
    }
    epoll_event evt = {0};
    evt.data.fd = fd;
    evt.events = event;
    if(epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, fd, &evt) < 0){
        // log_info
        return false;
    }
    return true;
}

bool Epoller::DelFd(int fd, uint32_t event)
{
    if(fd < 0){
        return false;
    }
    epoll_event evt = {0};
    if(epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, &evt) < 0){
        // log_info
        return false;
    }
    return true;
}

int Epoller::Wait(int timeout_ms)
{
    return epoll_wait(_epoll_fd, _events.data(), _events.size(), timeout_ms);
}

int Epoller::GetEventFd(int idx)
{
    assert(idx >= 0 && idx < _events.size());
    return _events[idx].data.fd;
}

uint32_t Epoller::GetEventType(int idx)
{
    assert(idx >= 0 && idx < _events.size());
    return _events[idx].events;
}
