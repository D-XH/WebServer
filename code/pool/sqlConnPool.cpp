#include "sqlConnPool.h"

SqlConnPool* SqlConnPool::Instance() {
    static SqlConnPool ins;
    return &ins;
}

void SqlConnPool::Init(const char* Host, uint16_t Port, const char* User, const char* Pwd, const char* DBName, int connSize) {
    assert(connSize > 0);
    for (int i = 0; i < connSize; ++i) {
        MYSQL* conn = nullptr;
        if (conn = mysql_init(conn), !conn) {
            LOG_ERROR("mysql init failed! [file:%s, line:%d]", __FILE__, __LINE__);
            assert(conn);
        }

        conn = mysql_real_connect(conn, Host, User, Pwd, DBName, Port, nullptr, 0);
        if (!conn) {
            LOG_ERROR("mysql connect failed! [file:%s, line:%d]", __FILE__, __LINE__);
            assert(conn);
        }

        assert(conn);
        connQue_.emplace(conn);
    }
    sem_init(&sem_, 0, connSize);
    isClose_ = false;
}

void SqlConnPool::Close() {
    std::lock_guard<std::mutex> locker(mtx_);
    while (!connQue_.empty()) {
        MYSQL* conn = connQue_.front();
        connQue_.pop();
        mysql_close(conn);
    }
    mysql_library_end();
    isClose_ = true;
}

MYSQL* SqlConnPool::GetConn() {
    if (connQue_.empty()) {
        LOG_WARN("SqlConnPool busy!");
        return nullptr;
    }

    sem_wait(&sem_);
    std::lock_guard<std::mutex> locker(mtx_);
    MYSQL* conn = connQue_.front();
    connQue_.pop();
    return conn;
}

void SqlConnPool::FreeConn(MYSQL* conn) {
    assert(conn);
    std::lock_guard<std::mutex> locker(mtx_);
    connQue_.emplace(conn);
    sem_post(&sem_);
}

bool SqlConnPool::IsClose() {
    std::lock_guard<std::mutex> locker(mtx_);
    return isClose_;
}

SqlConnPool::~SqlConnPool() {
    Close();
}
