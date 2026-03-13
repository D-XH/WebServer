#include "log.h"

Log::Log() : buff_(1024) {
    isClose_ = true;
    isAsync_ = false;

    fp_ = nullptr;
    writeThread_ = nullptr;
    deque_ = nullptr;

    lineCount_ = 0;
    cntPerDay = 0;
}

Log::~Log() {
    while (!deque_->Empty()) {
        // 唤醒消费者，处理剩下的日志信息
        deque_->Flush();
    }

    // 
    isClose_ = true;
    deque_->Close();
    writeThread_->join();
    if (fp_) {
        std::lock_guard<std::mutex> locker(mtx_);
        Flush();
        fclose(fp_);
    }
}

void Log::InitAsync_(int MaxQueueCapacity) {
    if (!MaxQueueCapacity) {
        isAsync_ = false;
        return;
    }

    isAsync_ = true;
    if (!deque_) {
        deque_ = std::make_unique<BlockQueue<std::string>>(MaxQueueCapacity);
        writeThread_ = std::make_unique<std::thread>(FlushLogThread);
    }
}

void Log::InitLogFile_() {
    char fileName[LOG_NAME_LEN] = { 0 };

    time_t timeStamp = time(nullptr);
    tm* systime = localtime(&timeStamp);

    lineCount_ = 0;
    cntPerDay = 0;
    toDay_ = systime->tm_mday;

    snprintf(fileName, LOG_NAME_LEN - 1,
        "%s/%04d_%02d_%02d-0%s",
        path_, systime->tm_year + 1900, systime->tm_mon + 1, systime->tm_mday, suffix_);

    std::lock_guard<std::mutex> locker(mtx_);
    buff_.RetrieveAll();
    if (fp_) {  // 已经打开，需要重新打开
        fclose(fp_);
    }
    fp_ = fopen(fileName, "a");
    if (fp_ == nullptr) {
        mkdir(path_, 0777);
        fp_ = fopen(fileName, "a");
    }
    assert(fp_ != nullptr);
}

// 加入日志等级
void Log::AppendLogLevelTitle_(LogLevel level) {
    switch (level) {
    case LogLevel::DEBUG:
        buff_.Append("[debug]: ", 9);
        break;
    case LogLevel::INFO:
        buff_.Append("[info] : ", 9);
        break;
    case LogLevel::WARN:
        buff_.Append("[warn] : ", 9);
        break;
    case LogLevel::ERROR:
        buff_.Append("[error]: ", 9);
        break;
    default:
        buff_.Append("[info] : ", 9);
        break;
    }
}

// 异步写
void Log::AsyncWrite() {
    std::string str = "";
    while (!IsClose()) {
        if (deque_->Pop(str) == false) continue;
        std::lock_guard<std::mutex> locker(mtx_);
        fputs(str.c_str(), fp_);
    }
}

Log* Log::Instance() {
    static Log ins_;
    return &ins_;
}

// 日志初始化
void Log::Init(LogLevel Level, const char* Path, const char* Suffix, int MaxQueueCapacity) {
    isClose_ = false;
    level_ = Level;
    path_ = Path;
    suffix_ = Suffix;

    // 初始化异步
    InitAsync_(MaxQueueCapacity);

    // 初始化日志文件
    InitLogFile_();
}

bool Log::IsClose() {
    return isClose_;
}

void Log::SetLevel(LogLevel Level) {
    std::lock_guard<std::mutex> locker(mtx_);
    level_ = Level;
}

LogLevel Log::GetLevel() {
    std::lock_guard<std::mutex> locker(mtx_);
    return level_;
}

void Log::Write(LogLevel Level, const char* format, ...) {
    timeval now = { 0, 0 };
    gettimeofday(&now, nullptr);    // 获取微秒级时间戳
    time_t tSec = now.tv_sec;
    tm* systime = localtime(&tSec);
    tm t = *systime;

    // 需要创建新的日志文件
    if (toDay_ != t.tm_mday || ((lineCount_ / LOG_MAX_LINE) != cntPerDay)) {
        std::unique_lock<std::mutex> locker(mtx_);
        if (toDay_ != t.tm_mday) {
            toDay_ = t.tm_mday;
            cntPerDay = 0;
            lineCount_ = 0;
        }
        else {
            cntPerDay = lineCount_ / LOG_MAX_LINE;
        }
        locker.unlock();

        char fileName[LOG_NAME_LEN] = { 0 };
        snprintf(fileName, LOG_NAME_LEN, "%s/%04d_%02d_%02d-%d%s",
            path_, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, cntPerDay, suffix_);

        locker.lock();
        Flush();
        fclose(fp_);
        fp_ = fopen(fileName, "a");
        assert(fp_ != nullptr);
    }

    // 写入buffer
    std::unique_lock<std::mutex> locker(mtx_);
    lineCount_++;

    // 写入时间
    buff_.EnsureWritable(128);
    int len = snprintf(buff_.BeginWrite(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
        t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
    buff_.HasWritten(len);

    // 写入日志等级
    AppendLogLevelTitle_(Level);

    va_list vaList;
    va_start(vaList, format);
    int n = vsnprintf(buff_.BeginWrite(), buff_.WritableBytes(), format, vaList);
    va_end(vaList);

    buff_.HasWritten(n);
    buff_.Append("\n\0", 2);

    if (isAsync_ && deque_ && !deque_->Full()) {
        // 异步方式，把缓冲区中数据放入阻塞队列
        deque_->PushBack(buff_.RetrieveAllToStr());
    }
    else {
        // 同步方式，直接把缓冲区中数据写入文件
        fputs(buff_.Peek(), fp_);
        buff_.RetrieveAll();
    }
}

void Log::Flush() {
    if (isAsync_) {
        // 异步刷新
        deque_->Flush();
    }
    // 同步刷新
    fflush(fp_);
}

void Log::FlushLogThread() {
    Instance()->AsyncWrite();
}
