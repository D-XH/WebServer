#include "server/webServer.h"

int main() {
    WebServer server(9909, TrigMode::ALL_ET, 60000, false,
        "localhost", 3306, "deng", "deng", "WebServer", 12,
        4, true, LogLevel::DEBUG, 1024);

    server.Start();
    return 0;
}