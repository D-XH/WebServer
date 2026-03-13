#include "httpConn.h"

const char* HttpConn::srcDir;
std::atomic_int HttpConn::userCount;
bool HttpConn::isET;

void HttpConn::PreInit(bool IsET) {
    HttpConn::srcDir = ConfigManager::Instance()->GetValue_server("srcDir", "");
    HttpConn::isET = IsET;
    HttpConn::userCount = 0;
}

HttpConn::HttpConn()
    : writeBuff_(), readBuff_(),
    request_(), respose_() {
    fd_ = -1;
    isClose_ = true;
    addr_ = { 0 };
}

HttpConn::~HttpConn() {
    Close();
}

void HttpConn::Init(int SockFd, const sockaddr_in& Addr) {
    assert(SockFd > 0);
    userCount.fetch_add(1, std::memory_order_relaxed);

    fd_ = SockFd;
    addr_ = Addr;
    writeBuff_.RetrieveAll();
    readBuff_.RetrieveAll();

    isClose_ = false;
    LOG_INFO("Client[%d](%s:%d) in, userCount:%d", fd_, GetIp(), GetPort(), userCount.load());
}

void HttpConn::Close() {
    respose_.UnmapFile();
    if (!isClose_) {
        isClose_ = true;
        userCount.fetch_sub(1, std::memory_order_relaxed);
        close(fd_);
        LOG_INFO("Client[%d](%s:%d) quit, userCount:%d", fd_, GetIp(), GetPort(), userCount.load());
    }
}

// 把缓冲区写入fd
ssize_t HttpConn::Write(int* SaveErrNo) {
    ssize_t len = -1;
    do {
        len = writev(fd_, iov_, 2);
        if (len <= 0) {
            *SaveErrNo = errno;
            break;
        }

        if (iov_[0].iov_len + iov_[1].iov_len == 0) {
            // 两个都已经读完
            break;
        }
        else if (static_cast<size_t>(len) > iov_[0].iov_len) {
            // 0(writeBuff_)读完了，1(file)还剩
            size_t tmp = len - iov_[0].iov_len;     // 1中已读字节
            iov_[1].iov_base = (char*)iov_[1].iov_base + tmp;
            iov_[1].iov_len -= tmp;
            if (iov_[0].iov_len) {
                writeBuff_.RetrieveAll();
                iov_[0].iov_len = 0;
            }
        }
        else {
            // 0(writeBuff_)没读完
            iov_[0].iov_base = (char*)iov_[0].iov_base + len;
            iov_[0].iov_len -= len;
            writeBuff_.Retrieve(len);
        }
    } while (isET);
    return len;
}

// 读取fd，写入缓冲区
ssize_t HttpConn::Read(int* SaveErrNo) {
    ssize_t len = -1;
    do {
        len = readBuff_.ReadFd(fd_, SaveErrNo);
        if (len <= 0) {
            break;
        }
    } while (isET); // 边缘触发要一次全读出
    return len;
}

bool HttpConn::Process() {
    request_.Init();
    if (readBuff_.ReadableBytes() <= 0) {
        return false;
    }
    else if (request_.Parse(readBuff_)) {
        // 解析成功
        LOG_DEBUG("%s", request_.GetPath().c_str());
        respose_.Init(srcDir, request_.GetPath(), request_.IsKeepAlive(), 200);
    }
    else {
        respose_.Init(srcDir, request_.GetPath(), false, 400);
    }

    // 生成响应头
    respose_.MakeResponse(writeBuff_);

    iov_[0].iov_base = const_cast<char*>(writeBuff_.Peek());
    iov_[0].iov_len = writeBuff_.ReadableBytes();
    iovCnt_ = 1;

    if (respose_.GetFileSize() && respose_.GetFileMMPtr()) {
        iov_[1].iov_base = respose_.GetFileMMPtr();
        iov_[1].iov_len = respose_.GetFileSize();
        iovCnt_ = 2;
    }
    LOG_DEBUG("file size: %d, iovCnt: %d, total size: %d ", respose_.GetFileSize(), iovCnt_, GetToWriteBytes());
    return true;
}

char* HttpConn::GetIp() {
    return inet_ntoa(addr_.sin_addr);
}

uint16_t HttpConn::GetPort() {
    return ntohs(addr_.sin_port);
}

int HttpConn::GetClientFd() {
    return fd_;
}

// 将要写到fd的总字节数
int HttpConn::GetToWriteBytes() {
    return iov_[0].iov_len + iov_[1].iov_len;
}

bool HttpConn::IsKeepAlive() {
    return request_.IsKeepAlive();
}
