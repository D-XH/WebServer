#pragma once

#include <fcntl.h>

#include "../http/httpConn.h"
#include "../epoller/epoller.h"
#include "../pool/threadPool.h"
#include "../pool/sqlConnPool.h"
#include "../timer/heapTimer.h"
#include "../log/log.h"

enum TrigMode {
    NO_ET,
    HTTP_ET,
    LISTEN_ET,
    ALL_ET
};

class WebServer {
public:
    WebServer(int Port, TrigMode TriMode, int Timeout, bool OptLinger,   // 
        const char* DBHost, int DBPort, const char* UserName, const char* Pwd, const char* DBName, int MaxDBConn, // 数据库连接初始化
        int ThreadNum,  // 线程池初始化
        bool OpenLog, LogLevel LLevel, int LQueueSize); // 日志初始化
    ~WebServer();

    void Start();
private:
    void InitEventMode_(TrigMode TriMode);
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

    char* srcDir_;
    bool optLinger_;
    uint32_t listenEvtMode_;
    uint32_t httpConnEvtMode_;

    ThreadPool threadPool_;
    HeapTimer timer_;
    Epoller epoller_;
    std::unordered_map<int, HttpConn> users_;
};