#include "../code/pool/sqlConnPool.h"
#include "../code/pool/sqlConnRAII.h"
#include "../code/pool/threadPool.h"
#include "../code/log/log.h"
#include <iostream>

int main() {
    Log::Instance()->Init(INFO, "./testlog3");
    SqlConnPool::Instance()->Init("localhost", 3306, "deng", "deng", "WebServer", 4);
    {
        SqlConnRAII connRAII;
        SqlConnRAII connRAII2;
        mysql_query(connRAII.GetConn(), "select * from tbl_test;");
    }
    SqlConnRAII connRAII3;

    sleep(3);
    return 0;
}
