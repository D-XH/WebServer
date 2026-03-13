#include "epoller.h"

Epoller::Epoller(int MaxEvent) : events_(MaxEvent) {
    epFd_ = epoll_create1(0);
    if (epFd_ == -1) {
        LOG_ERROR("Epoller create failed! errno:%d.", errno);
    }
}

Epoller::~Epoller() {
    close(epFd_);
}

void Epoller::AddFd(int Fd, uint32_t EventMode) {
    if (Fd < 0) return;
    epoll_event evt;
    evt.data.fd = Fd;
    evt.events = EventMode;
    if (epoll_ctl(epFd_, EPOLL_CTL_ADD, Fd, &evt) == -1) {
        LOG_ERROR("AddFd %d failed! evetMode: %d.", Fd, EventMode);
    }
}

void Epoller::ModFd(int Fd, uint32_t EventMode) {
    if (Fd < 0) return;
    epoll_event evt;
    evt.data.fd = Fd;
    evt.events = EventMode;
    if (epoll_ctl(epFd_, EPOLL_CTL_MOD, Fd, &evt) == -1) {
        LOG_ERROR("ModFd %d failed! evetMode: %d.", Fd, EventMode);
    }
}

void Epoller::DelFd(int Fd) {
    if (Fd < 0) return;
    if (epoll_ctl(epFd_, EPOLL_CTL_DEL, Fd, nullptr) == -1) {
        LOG_ERROR("DelFd %d failed!", Fd);
    }
}

int Epoller::Wait(int Timeout) {
    return epoll_wait(epFd_, events_.data(), events_.size(), Timeout);
}

int Epoller::GetEventFd(size_t idx) const {
    return events_[idx].data.fd;
}

uint32_t Epoller::GetEventMode(size_t idx) const {
    return events_[idx].events;
}


