#include "webserver.h"

WebServer::WebServer(int server_port, 
                int sql_port, const char* sql_host, const char* db_name, const char* sql_user, const char* sql_pwd,
                int thrads_size, int conn_size,
                int trigMode, bool opt_linger, int timeout_ms,
                bool open_log, int log_level, int log_queSize)
: _is_open(true), _listen_port(server_port)
{
    // http客户连接配置初始化

    // 数据库连接池初始化

    // 定时器初始化
    if(timeout_ms > 0){

    }

    // 设置监听socket和连接socket的触发事件
    _initEventMode(trigMode);

    // 初始化监听socket，并添加到epoll中监听。
    if(!_initSocket()){
        _is_open = false;
    }

    // 日志初始化
    if(open_log){
        // 开启日志
    }
}

bool WebServer::_initSocket()
{

    if(_listen_port < 1024 || _listen_port > 65535){
        // log_err
        return false;
    }

    // 初始化ip和端口
    sockaddr_in addr;
    addr.sin_port = htons(_listen_port);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // 初始化连接socket
    _listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(_listen_fd < 0){
        // log_err
        return false;
    }

    // 发送完再关闭连接选项
    linger _opt_linger{0};
    if(_linger){
        _opt_linger.l_linger = 1;
        _opt_linger.l_onoff = 1;
    }
    if(setsockopt(_listen_fd, SOL_SOCKET, SO_LINGER, &_opt_linger, sizeof(_opt_linger)) < 0){
        // log
        close(_listen_fd);
        return false;
    }

    // 地址重用，避免time_wait
    int _opt_val = 1;
    if(setsockopt(_listen_fd, SOL_SOCKET, SO_REUSEADDR, &_opt_val, sizeof(_opt_val)) < 0){
        // log
        close(_listen_fd);
        return false;
    }

    // 设置为非阻塞模式
    _setNonBlock(_listen_fd);

    // 绑定ip和端口
    if(bind(_listen_fd, (sockaddr*)&addr, sizeof(addr)) < 0){
        // log_err
        close(_listen_fd);
        return false;
    }

    if(listen(_listen_fd, 256) < 0){
        // log_err
        close(_listen_fd);
        return false;
    }

    if(_epoller->AddFd(_listen_fd, _listen_evt | EPOLLIN) == false){
        // log_err
        close(_listen_fd);
        return false;
    }

    // log_info
    return true;
}

void WebServer::_initEventMode(int trigMode)
{
    _listen_evt = EPOLLRDHUP;
    _conn_evt = EPOLLONESHOT | EPOLLRDHUP;
    switch (trigMode)
    {
    case 0:
        break;
    case 1:
        _conn_evt |= EPOLLET;
        break;
    case 2:
        _listen_evt |= EPOLLET;
        break;
    case 3:
        _conn_evt |= EPOLLET;
        _listen_evt |= EPOLLET;
        break;
    default:
        _conn_evt |= EPOLLET;
        _listen_evt |= EPOLLET;
        break;
    }

}

void WebServer::_setNonBlock(int fd)
{
    fcntl(fd, F_SETFL, O_NONBLOCK | fcntl(fd, F_GETFL));
}
