#include <unistd.h>
#include <string>
#include <iostream>
#include "server/webServer.h"

int main() {
    std::string cwd(__FILE__);
    size_t pos = cwd.find_last_of('/');
    cwd = cwd.substr(0, pos);

    WebServer server((cwd + "/config.ini"));

    server.Start();
    return 0;
}