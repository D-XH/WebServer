#include "Log.h"

Log* Log::GetInstance(){
    static Log _log;
    return &_log;
}

Log::Log()
{
    _isOpen = false;
    _level = INFO;

    _log_fp = nullptr;
    _max_lines = 50000;
    _cur_line = 0;
    _today = 0;

    _isAsync = false;
    _async_thread = nullptr;

    _BQ = nullptr;
}

Log::~Log()
{
    
    if(_isOpen){
        if(_async_thread && _async_thread->joinable()){
            while(!_BQ->Empty()){
                _BQ->Flush();
            }
            _BQ->Close();
            _async_thread->join();
        }
        if(_log_fp){
            lock_guard<mutex> lock(_mtx);
            Flush();
            fclose(_log_fp);
        }
        lock_guard<mutex> lock(_mtx);
        _isAsync = false;
        _isOpen = false;
    }
}


void Log::Init(LogLevel level, const char* path, const char* suffix, int queue_capacity)
{
    SetLevel(level);

    if(!_isOpen && queue_capacity > 0){
        _BQ.reset(new BlockQueue<string>(queue_capacity));
        _async_thread.reset(new thread(_async_thread_func));
        _isAsync = true;
    }
    if(_isAsync && queue_capacity > _BQ->Capacity()) _BQ->Resize(queue_capacity);

    _isOpen = true;
    _cur_line = 0;
    
    time_t timer = time(nullptr);
    tm* sysTime = localtime(&timer);

    _today = sysTime->tm_mday;
    _path = path;
    _suffix = suffix;

    char filepath[256] = {0};
    snprintf(filepath, 255, "%s/%04d_%02d_%02d%s", path, sysTime->tm_year + 1900, sysTime->tm_mon + 1, sysTime->tm_mday, suffix);

    if(_log_fp){
        FlushAll();
        fclose(_log_fp); 
    }
    _log_fp = fopen(filepath, "a");
    if(_log_fp == nullptr){
        mkdir(path, 0777);
        _log_fp = fopen(filepath, "a");
    }
    assert(_log_fp != nullptr);

}

int Log::GetLevel()
{
    lock_guard<mutex> lock(_mtx);
    return _level;
}

void Log::SetMaxLines(int n)
{
    lock_guard<mutex> lock(_mtx);
    _max_lines = n;
}
#include<iostream>
void Log::Write(LogLevel level, const char *format, ...)
{
    assert(_isOpen);
    if(level > _level){
        return;
    }

    timespec ts;
    clock_gettime(0, &ts);
    tm* sysTime = localtime(&ts.tv_sec);

    {
        lock_guard<mutex> lock(_mtx);
        // 新的一天用新的日志进行记录。同一天记录行数超过限制，进行分文件记录。
        if(_today != sysTime->tm_mday || (_cur_line && (_cur_line % _max_lines) == 0)){
            char filepath[256] = {0};
            char prefix[36] = {0};
            snprintf(prefix, 35, "%04d_%02d_%02d", sysTime->tm_year+1900, sysTime->tm_mon+1, sysTime->tm_mday);

            if(_today != sysTime->tm_mday){
                snprintf(filepath, 255, "%s/%s%s", _path, prefix, _suffix);
                _today = sysTime->tm_mday;
                _cur_line = 0;
            }else{
                snprintf(filepath, 255, "%s/%s-%d%s", _path, prefix, (_cur_line / _max_lines), _suffix);
            }
            
            FlushAll();
            fclose(_log_fp);
            _log_fp = fopen(filepath, "a");
            assert(_log_fp != nullptr);
        }
    }

    {
        lock_guard<mutex> lock(_mtx);
        // 写入时间：年月日 时分秒.纳秒
        int n = snprintf(_buf.WriteBeginPtr(), 128, "%d %d-%02d-%02d %02d:%02d:%02d.%06ld ",
                    _cur_line++,
                    sysTime->tm_year+1900, sysTime->tm_mon+1, sysTime->tm_mday, 
                    sysTime->tm_hour, sysTime->tm_min, sysTime->tm_sec, ts.tv_nsec);
        _buf.ForwardWritePos(n);
        
        // 写日志的等级：DEBUG、INFO
        _appendLevelTitle(level);

        // 写具体日志信息
        va_list _vaList;
        va_start(_vaList, format);
        int m = vsnprintf(_buf.WriteBeginPtr(), _buf.WriteableBytes(), format, _vaList);
        va_end(_vaList);
        _buf.ForwardWritePos(m);

        // 加入换行和c字符串结束符
        _buf.Append("\n\0", 2);

        if(_isAsync && _BQ && !_BQ->Full()){
            // 异步写。压入阻塞队列，异步线程写。
            string s = _buf.RetriveAllToStr();
            _BQ->Push(s);
        }else{
            // 同步写。
            fputs(_buf.ReadBeginPtr(), _log_fp);
        }
        _buf.RetriveAll();
    }
}

void Log::Flush(){
    if(_isAsync){
        _BQ->Flush();
    }
    fflush(_log_fp);
}

void Log::FlushAll()
{
    if(_isAsync){
        while(!_BQ->Empty()){
            _BQ->Flush();
        }
    }
    fflush(_log_fp);
}

bool Log::IsOpen()
{
    lock_guard<mutex> lock(_mtx);
    return _isOpen;
}

void Log::SetLevel(LogLevel level)
{
    lock_guard<mutex> lock(_mtx);
    _level = level;
}

void Log::_async_write()
{
    string str = "";
    while(1){
        _BQ->Pop(&str);
        if(!_BQ->IsClosed()){
            // lock_guard<mutex> lock(_mtx);
            fputs(str.c_str(), _log_fp);
        }else{
            break;
        }
    }
}

void Log::_appendLevelTitle(LogLevel level)
{
    switch (level)
    {
    case DEBUG:
        _buf.Append("[DEBUG]: ", 9);
        break;
    case INFO:
        _buf.Append("[INFO] : ", 9);
        break;
    case WARN:
        _buf.Append("[WARN] : ", 9);
        break;
    case ERROR:
        _buf.Append("[ERROR]: ", 9);
        break;
    default:
        _buf.Append("[INFO] : ", 9);
        break;
    }
}

void Log::_async_thread_func()
{
    Log::GetInstance()->_async_write();
}
