#include "webServer.h"

WebServer::WebServer(const std::string& ConfigPath)  // 日志初始化
    :
    threadPool_(4),
    timer_(),
    epoller_() {

    ConfigManager::Instance()->Init(ConfigPath.c_str());

    InitEventMode_();
    listenPort_ = ConfigManager::Instance()->GetValue_server("port", 9909);
    httpTimeout_ = ConfigManager::Instance()->GetValue_server("timeOut", -1);

    // 日志初始化
    LogLevel LLevel = static_cast<LogLevel>(ConfigManager::Instance()->GetValue_log("level", 1));
    const char* logPath = ConfigManager::Instance()->GetValue_log("path", "./logFiles");
    const char* suffix = ConfigManager::Instance()->GetValue_log("suffix", ".log");
    int LQueueSize = static_cast<int>(ConfigManager::Instance()->GetValue_log("queueSize", 1024));
    Log::Instance()->Init(LLevel, logPath, suffix, LQueueSize);

    // 数据库连接初始化
    const char* host = ConfigManager::Instance()->GetValue_database("host", "localhost");
    uint16_t port = static_cast<uint16_t>(ConfigManager::Instance()->GetValue_database("port", 3306));
    const char* username = ConfigManager::Instance()->GetValue_database("userName", "deng");
    const char* password = ConfigManager::Instance()->GetValue_database("password", "deng");
    const char* dbName = ConfigManager::Instance()->GetValue_database("dbName", "WebServer");
    long maxDBConn = ConfigManager::Instance()->GetValue_database("maxDBConn", 16);
    SqlConnPool::Instance()->Init(host, port, username, password, dbName, maxDBConn);

    // socket初始化，建立监听
    if (!InitSocket_()) {
        isClose_ = true;
        LOG_ERROR("========== Server init error!==========")
    }
    else {
        isClose_ = false;
        LOG_INFO("========== Server init ==========");
        LOG_INFO("Port:%d", listenPort_);
        LOG_INFO("Listen Mode: %s, OpenConn Mode: %s",
            (listenEvtMode_ & EPOLLET ? "ET" : "LT"),
            (httpConnEvtMode_ & EPOLLET ? "ET" : "LT"));
        LOG_INFO("LogSys level: %d", LLevel);
        LOG_INFO("srcDir: %s", HttpConn::srcDir);
    }
}

WebServer::~WebServer() {
    close(listenFd_);
    isClose_ = true;
    SqlConnPool::Instance()->Close();
}

void WebServer::Start() {
    if (!isClose_) { LOG_INFO("========== Server start =========="); }

    int tickTime = -1;
    while (!isClose_) {
        // 主循环
        if (httpTimeout_ > 0) {
            tickTime = timer_.GetNextTick();
        }
        int trigNum = epoller_.Wait(tickTime);
        for (int i = 0; i < trigNum; ++i) {
            int fd = epoller_.GetEventFd(i);
            uint32_t eventMode = epoller_.GetEventMode(i);
            if (fd == listenFd_) {
                DealListen_();
            }
            else if (eventMode & (EPOLLHUP | EPOLLRDHUP | EPOLLERR)) {
                DelClient_(fd);
            }
            else if (eventMode & EPOLLIN) {
                DealRead_(fd);
            }
            else if (eventMode & EPOLLOUT) {
                DealWrite_(fd);
            }
            else {
                LOG_ERROR("Unexpected event");
            }
        }
    }
}

void WebServer::InitEventMode_() {
    listenEvtMode_ = EPOLLRDHUP;    // 监听socket检测到读挂起（即对端关闭），触发
    httpConnEvtMode_ = EPOLLONESHOT | EPOLLRDHUP;    // http socket检测到读挂起触发，且每次触发后需要重新添加到epoll
    TrigMode trigerMode = static_cast<TrigMode>(ConfigManager::Instance()->GetValue_server("trigMode", 3));
    switch (trigerMode) {
    case TrigMode::NO_ET:
        break;
    case TrigMode::HTTP_ET:
        httpConnEvtMode_ |= EPOLLET;
        break;
    case TrigMode::LISTEN_ET:
        listenEvtMode_ |= EPOLLET;
        break;
    case TrigMode::ALL_ET:
        httpConnEvtMode_ |= EPOLLET;
        listenEvtMode_ |= EPOLLET;
        break;
    default:
        httpConnEvtMode_ |= EPOLLET;
        listenEvtMode_ |= EPOLLET;
        break;
    }
    HttpConn::PreInit(httpConnEvtMode_ & EPOLLET);
}

void WebServer::SetSockLinger() {
    if (listenFd_ < 0) return;

    linger optVal{ 1, 1 };
    if (setsockopt(listenFd_, SOL_SOCKET, SO_LINGER, &optVal, sizeof(optVal)) < 0) {
        LOG_ERROR("ListenFd set option linger error!");
    }
}

void WebServer::SetSockAddrReuse() {
    if (listenFd_ < 0) return;

    int optVal = 1;
    if (setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(optVal)) < 0) {
        LOG_ERROR("ListenFd set option reuse addr error!");
    }
}

void WebServer::SetFdNonblock(int Fd) {
    int flag = fcntl(Fd, F_GETFL, 0);
    fcntl(Fd, F_SETFL, flag | O_NONBLOCK);
}

bool WebServer::InitSocket_() {

    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd_ < 0) {
        LOG_ERROR("Socket create failed!");
        return false;
    }

    SetSockAddrReuse();
    if (ConfigManager::Instance()->GetValue_server("optLinger", false)) SetSockLinger();

    SetFdNonblock(listenFd_);

    sockaddr_in bindAddr;
    bindAddr.sin_family = AF_INET;
    bindAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    bindAddr.sin_port = htons(listenPort_);
    if (bind(listenFd_, reinterpret_cast<sockaddr*>(&bindAddr), sizeof(bindAddr)) < 0) {
        LOG_ERROR("Listen fd bind addr error! fd:%d, ip:%s, port:%d.", listenFd_, inet_ntoa(bindAddr.sin_addr), listenPort_);
        close(listenFd_);
        return false;
    }

    if (listen(listenFd_, 6) < 0) {
        LOG_ERROR("ListenFd listen error! errno:%d.", errno);
        close(listenFd_);
        return false;
    }

    epoller_.AddFd(listenFd_, listenEvtMode_ | EPOLLIN);
    LOG_INFO("Socket init successful! listen fd:%d, port:%d, trigerMode:%d", listenFd_, listenPort_, listenEvtMode_);
    return true;
}

void WebServer::DealListen_() {
    sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    do {
        int clientFd = accept(listenFd_, reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);
        if (clientFd <= 0) { return; }
        else if (HttpConn::userCount >= MAX_HTTP_CONN) {

            LOG_WARN("Client over MAX_HTTP_CONN[%d]! ", MAX_HTTP_CONN);
            return;
        }
        AddClient_(clientFd, clientAddr);
    } while (listenEvtMode_ & EPOLLET);
}

void WebServer::DealRead_(int ClientFd) {
    assert(ClientFd > 0);
    assert(users_.count(ClientFd) > 0);

    if (httpTimeout_) timer_.Adjust(ClientFd, httpTimeout_);
    threadPool_.AddTask(std::bind(&WebServer::OnRead_, this, &users_[ClientFd]));
}

void WebServer::DealWrite_(int ClientFd) {
    assert(ClientFd > 0);
    assert(users_.count(ClientFd) > 0);

    if (httpTimeout_) timer_.Adjust(ClientFd, httpTimeout_);
    threadPool_.AddTask(std::bind(&WebServer::OnWrite_, this, &users_[ClientFd]));
}

void WebServer::AddClient_(int ClientFd, sockaddr_in& ClientAddr) {
    assert(ClientFd > 0);
    SetFdNonblock(ClientFd);

    users_[ClientFd].Init(ClientFd, ClientAddr);
    epoller_.AddFd(ClientFd, httpConnEvtMode_ | EPOLLIN);
    if (httpTimeout_ > 0) {
        // 超时断开连接
        timer_.Add(ClientFd, httpTimeout_, std::bind(&WebServer::DelClient_, this, ClientFd));
    }
}

void WebServer::DelClient_(int ClientFd) {
    assert(ClientFd > 0);
    assert(users_.count(ClientFd) > 0);
    LOG_INFO("Client[%d] quit!", ClientFd);

    epoller_.DelFd(ClientFd);
    users_[ClientFd].Close();
}

void WebServer::OnRead_(HttpConn* Client) {
    int readErrNo = 0;
    ssize_t readSize = Client->Read(&readErrNo);
    if (readSize <= 0 && readErrNo != EAGAIN) {
        DelClient_(Client->GetClientFd());
        return;
    }
    // 把http请求读到缓冲区后，再进行处理
    Process_(Client);
}

void WebServer::OnWrite_(HttpConn* Client) {
    int writeErrNo = 0;

    // 把http响应写入ClientFd，返回响应
    ssize_t writeSize = Client->Write(&writeErrNo);
    if (Client->GetToWriteBytes() == 0) {
        // 写完了，重新变成可读
        if (Client->IsKeepAlive()) {
            epoller_.ModFd(Client->GetClientFd(), httpConnEvtMode_ | EPOLLIN);
            return;
        }
    }
    else if (writeSize < 0) {   // 
        if (writeErrNo == EAGAIN) { // 缓冲区满了，继续写
            epoller_.ModFd(Client->GetClientFd(), httpConnEvtMode_ | EPOLLOUT);
            return;
        }
    }
    // 不保持连接写完或其他问题，断开连接
    DelClient_(Client->GetClientFd());
}

void WebServer::Process_(HttpConn* Client) {
    // 1. 解析http请求
    // 2. 生成http响应
    // 3. 把http响应写入写缓冲
    if (Client->Process()) {
        epoller_.ModFd(Client->GetClientFd(), httpConnEvtMode_ | EPOLLOUT);
    }
    else {
        // 因为是oneshot，所以必须再次设置为EPOLLIN或者EPOLLOUT，wait时才会再次加入等待列表
        epoller_.ModFd(Client->GetClientFd(), httpConnEvtMode_ | EPOLLIN);
    }
}
