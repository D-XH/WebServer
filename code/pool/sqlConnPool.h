#pragma once

#include <mysql/mysql.h>
#include <queue>
#include <mutex>
#include <thread>
#include <semaphore.h>
#include <assert.h>
#include "../log/log.h"

class SqlConnPool {
public:
    static SqlConnPool* Instance();
    void Init(const char* Host, uint16_t port,
        const char* User, const char* Pwd,
        const char* DBName, int connSize);
    void Close();

    MYSQL* GetConn();
    void FreeConn(MYSQL* conn);

    bool IsClose();
private:
    SqlConnPool() :isClose_(true) {}
    ~SqlConnPool();

private:
    bool isClose_;
    std::queue<MYSQL*> connQue_;

    sem_t sem_;
    std::mutex mtx_;
};