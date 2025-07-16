#ifndef __LOG_H__
#define __LOG_H__

#include<string>
#include<memory>
#include<thread>
#include<mutex>
#include<functional>
#include<time.h>
#include<stdio.h>
#include<sys/stat.h>
#include<stdarg.h>
#include"../Buffer/Buffer.h"
#include"BlockQueue.hpp"

using std::string;
using std::thread;
using std::mutex;
using std::unique_ptr;
using std::lock_guard;

enum LogLevel{
    DEBUG,
    INFO,
    WARN,
    ERROR 
};

struct Log
{
public:
    static Log* GetInstance();

    void Init(LogLevel level, const char* path = "./log", const char* suffix = ".log", int queue_capacity = 1024);

    void SetLevel(LogLevel level);
    int GetLevel();
    void SetMaxLines(int n);

    void Write(LogLevel level, const char* format, ...);
    void Flush();
    void FlushAll();

    bool IsOpen();
private:
    Log();
    ~Log();

    void _async_write();
    void _appendLevelTitle(LogLevel level);
    static void _async_thread_func();
private:
    unique_ptr<BlockQueue<string>> _BQ;
    Buffer _buf;

    unique_ptr<thread> _async_thread;
    mutex _mtx;
private:
    const char*  _suffix;
    const char*  _path;

    LogLevel _level;
    bool _isOpen;
    bool _isAsync;
    FILE* _log_fp;

    int _today;
    int _max_lines;
    int _cur_line;

};

#define LOG_DEBUG(format, ...) {Log::GetInstance()->Write(DEBUG, format, ##__VA_ARGS__);}
#define LOG_INFO(format, ...) {Log::GetInstance()->Write(INFO, format, ##__VA_ARGS__);}
#define LOG_WARN(format, ...) {Log::GetInstance()->Write(WARN, format, ##__VA_ARGS__);}
#define LOG_ERR(format, ...) {Log::GetInstance()->Write(ERROR, format, ##__VA_ARGS__);}
#endif