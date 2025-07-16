#ifndef __EPOLLER_H__
#define __EPOLLER_H__

#include<sys/epoll.h>
#include<assert.h>
#include<unistd.h>
#include<vector>
using std::vector;

struct Epoller
{
public:
    Epoller(size_t max_events);
    ~Epoller();
    bool AddFd(int fd, uint32_t event);     // 添加fd到epoll监听中
    bool ModFd(int fd, uint32_t event);     // 修改fd的事件类型
    bool DelFd(int fd, uint32_t event);     // 取消对fd时间的监听
    int Wait(int timeout_ms);               // 监听
    int GetEventFd(int idx);                // 根据下标获取事件的fd
    uint32_t GetEventType(int idx);         // 根据下标获取时间的类型
private:
    int _epoll_fd;                 
    vector<epoll_event> _events;    // 就绪事件数组
};

#endif