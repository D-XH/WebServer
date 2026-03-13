#pragma once

#include "sqlConnPool.h"

class SqlConnRAII {
public:
    SqlConnRAII() {
        if (!SqlConnPool::Instance()->IsClose()) {
            conn = SqlConnPool::Instance()->GetConn();
        }
    }
    ~SqlConnRAII() {
        if (conn) { SqlConnPool::Instance()->FreeConn(conn); }
    }

    MYSQL* GetConn() {
        assert(conn);
        return conn;
    };
private:
    MYSQL* conn;
};