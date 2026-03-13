#pragma once

#include <fcntl.h>

#include "configManager.h"
#include "../http/httpConn.h"
#include "../epoller/epoller.h"
#include "../pool/threadPool.h"
#include "../pool/sqlConnPool.h"
#include "../timer/heapTimer.h"
#include "../log/log.h"

enum TrigMode {
    NO_ET = 0,
    HTTP_ET = 1,
    LISTEN_ET = 2,
    ALL_ET = 3
};

class WebServer {
public:
    WebServer(const std::string& ConfigPath); // 日志初始化
    ~WebServer();

    void Start();
private:
    void InitEventMode_();
    bool InitSocket_();

    void DealListen_();
    void DealRead_(int ClientFd);
    void DealWrite_(int ClientFd);

    void AddClient_(int ClientFd, sockaddr_in& ClientAddr);
    void DelClient_(int ClientFd);

    void OnRead_(HttpConn* Client);
    void OnWrite_(HttpConn* Client);
    void Process_(HttpConn* Client);

    void SetSockLinger();
    void SetSockAddrReuse();

    void SetFdNonblock(int Fd);
private:
    static const int MAX_HTTP_CONN = 65536;
private:
    bool isClose_;

    int listenFd_;
    uint16_t listenPort_;
    int httpTimeout_;

    uint32_t listenEvtMode_;
    uint32_t httpConnEvtMode_;

    ThreadPool threadPool_;
    HeapTimer timer_;
    Epoller epoller_;
    std::unordered_map<int, HttpConn> users_;
};