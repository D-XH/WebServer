#pragma once

#include <atomic>

#include <sys/uio.h>
#include <arpa/inet.h>
#include <assert.h>

#include "httpRequest.h"
#include "httpResponse.h"
#include "../buffer/buffer.h"
#include "../log/log.h"
#include "../server/configManager.h"

class HttpConn {
public:
    static void PreInit(bool IsET);
public:
    HttpConn();
    ~HttpConn();

    void Init(int SockFd, const sockaddr_in& Addr);
    void Close();

    ssize_t Write(int* SaveErrNo);
    ssize_t Read(int* SaveErrNo);
    bool Process();

    char* GetIp();
    uint16_t GetPort();
    int GetClientFd();
    int GetToWriteBytes();
    bool IsKeepAlive();
public:
    static bool isET;
    static const char* srcDir;
    static std::atomic_int userCount;

private:
    bool isClose_;

    int fd_;
    sockaddr_in addr_;

    int iovCnt_;
    iovec iov_[2];  // 指向写缓冲和文件（如果有的话）
    Buffer readBuff_, writeBuff_;

    HttpRequest request_;
    HttpResponse respose_;
};
