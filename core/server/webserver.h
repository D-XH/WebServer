#ifndef __WEB_SERVER_H__
#define __WEB_SERVER_H__

#include<memory>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<fcntl.h>
#include"../Epoller/Epoller.h"
#include"../ThreadPool/ThreadPool.h"

struct WebServer
{
public:
    WebServer(int server_port, 
                int sql_port, const char* sql_host, const char* db_name, const char* sql_user, const char* sql_pwd,
                int thrads_size, int conn_size,
                int trigMode, bool opt_linger, int timeout_ms,
                bool open_log, int log_level, int log_queSize);
    void start();
private:
    bool _initSocket();
    void _initEventMode(int trigMode);
    void _addClient(int fd, sockaddr_in addr);
    void _setNonBlock(int fd);
private:
    std::unique_ptr<Epoller> _epoller;
    std::unique_ptr<ThreadPool> _thread_pool;
private:
    bool _is_open;

    uint32_t _listen_evt;
    uint32_t _conn_evt;

    int _listen_fd;
    int _listen_port;
    int _timeout_ms;

    bool _linger;
};

#endif