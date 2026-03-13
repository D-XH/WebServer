#pragma once

#include <unistd.h>
#include <thread>
#include <atomic>
#include <stdio.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/stat.h>
#include "blockQueue.h"
#include "../buffer/buffer.h"

enum LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3
};

class Log {
public:
    static Log* Instance();

    void Init(LogLevel Level, const char* Path = "./logFile", const char* Suffix = ".log", int MaxQueueCapacity = 1024);
    bool IsClose();

    void SetLevel(LogLevel Level);
    LogLevel GetLevel();

    void Write(LogLevel Level, const char* format, ...);
    void Flush();

    static void FlushLogThread();   // 异步写日志
private:
    Log();
    ~Log();

    void InitAsync_(int MaxQueueCapacity);
    void InitLogFile_();

    void AppendLogLevelTitle_(LogLevel level);
    void AsyncWrite();

private:
    const char* path_;
    const char* suffix_;

    const int LOG_PATH_LEN = 256;
    const int LOG_NAME_LEN = 256;
    const int LOG_MAX_LINE = 50000;
private:
    std::atomic_bool isClose_;
    FILE* fp_;  // 日志文件指针
    LogLevel level_;         // 日志输出级别
    Buffer buff_;       // 日志内容缓冲区

    int lineCount_;     // 当前日志已经记录行数
    int toDay_;         // 
    int cntPerDay;      // 当天第几个文件

    // 异步日志
    bool isAsync_;      // 是否开启异步
    std::mutex mtx_;    // 多线程锁
    std::unique_ptr<std::thread> writeThread_;  // 写日志线程
    std::unique_ptr<BlockQueue<std::string>> deque_;    // 阻塞队列

};

#define LOG_BASE(level, format, ...) \
    do {\
        Log* log = Log::Instance();\
        if(!log->IsClose() && log->GetLevel() <= level) {\
            log->Write(level, format, ##__VA_ARGS__);\
            log->Flush();\
        }\
    } while(0);

#define LOG_DEBUG(format, ...) do {LOG_BASE(LogLevel::DEBUG, format, ##__VA_ARGS__)}while(0);
#define LOG_INFO(format, ...) do {LOG_BASE(LogLevel::INFO, format, ##__VA_ARGS__)}while(0);
#define LOG_WARN(format, ...) do {LOG_BASE(LogLevel::WARN, format, ##__VA_ARGS__)}while(0);
#define LOG_ERROR(format, ...) do {LOG_BASE(LogLevel::ERROR, format, ##__VA_ARGS__)}while(0);