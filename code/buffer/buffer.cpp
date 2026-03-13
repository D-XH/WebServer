#include "buffer.h"

Buffer::Buffer(int initialBufferSize) : buffer_(initialBufferSize), readPos_(0), writePos_(0) {
}

// 剩余可写字节
size_t Buffer::WritableBytes() const {
    return buffer_.size() - writePos_;
}

// 剩余可读字节
size_t Buffer::ReadableBytes() const {
    return writePos_ - readPos_;
}

// 可回收字节，即已读字节
size_t Buffer::PrependableBytes() const {
    return readPos_;
}

// 预览可读的数据
const char* Buffer::Peek() const {
    return &buffer_[readPos_];
}

// 确保能够写入len字节数据，不够就进行扩展
void Buffer::EnsureWritable(size_t len) {
    if (len > WritableBytes()) {
        MakeSpace_(len);
    }
    assert(len <= WritableBytes());
}

// 已写len字节数据，移动writePos下标
void Buffer::HasWritten(size_t len) {
    writePos_.fetch_add(len, std::memory_order_relaxed);
}

// 回收len字节数据，移动readPos下标
void Buffer::Retrieve(size_t len) {
    readPos_.fetch_add(len, std::memory_order_relaxed);
}

// 回收到end的数据
void Buffer::RetrieveUntil(const char* end) {
    assert(Peek() < end);
    Retrieve(end - Peek());
}

// 回收所有数据，直接清空buffer
void Buffer::RetrieveAll() {
    buffer_.clear();
    // bzero(&buffer_[0], buffer_.size());
    readPos_ = writePos_ = 0;
}

// 回收所有数据，并把剩余可读传回
std::string Buffer::RetrieveAllToStr() {
    std::string ret(Peek(), ReadableBytes());
    RetrieveAll();
    return ret;
}

// 写指针位置
const char* Buffer::BeginWriteConst() const {
    return &buffer_[writePos_];
}

char* Buffer::BeginWrite() {
    return &buffer_[writePos_];
}

// c++字符串
void Buffer::Append(const std::string& str) {
    Append(str.c_str(), str.size());
}

// 从str写入len字节数据
void Buffer::Append(const char* str, size_t len) {
    assert(str);    // 不为空
    EnsureWritable(len);
    std::copy(str, str + len, BeginWrite());
    HasWritten(len);
}

// 写入len字节二进制数据
void Buffer::Append(const void* data, size_t len) {
    Append(static_cast<const char*>(data), len);
}

// 写入另一个buffer的数据
void Buffer::Append(const Buffer& buff) {
    Append(buff.Peek(), buff.ReadableBytes());
}

// 从文件描述符Fd读取数据，写入buffer
ssize_t Buffer::ReadFd(int Fd, int* ErrNo) {
    char buff[65535];   // 栈区
    size_t writable = WritableBytes();

    iovec iov[2];
    iov[0].iov_base = BeginWrite();
    iov[0].iov_len = writable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    ssize_t len = readv(Fd, iov, 2);
    if (len < 0) {
        *ErrNo = errno;
    }
    else if (static_cast<size_t>(len) < writable) {
        // 剩余可写字节数足够，直接移动writePos下标
        HasWritten(len);
    }
    else {
        // 读入字节超过buffer大小，多余存入buff
        writePos_ = buffer_.size();
        Append(buff, static_cast<size_t>(len - writable));
    }
    return len;
}

// 把buffer中数据写到文件描述符Fd中
ssize_t Buffer::WriteFd(int Fd, int* ErrNO) {
    ssize_t len = write(Fd, Peek(), ReadableBytes());
    if (len < 0) {
        *ErrNO = errno;
    }
    else {
        Retrieve(len);
    }
    return len;
}

// buffer首地址
char* Buffer::BeginPtr() {
    return &buffer_[0];
}

// 扩容
void Buffer::MakeSpace_(size_t len) {
    if (WritableBytes() + PrependableBytes() < len) {
        // 回收已读后剩余可写字节数小于len，需要扩容。
        buffer_.resize(writePos_ + len + 1);
    }
    else {
        size_t readable = ReadableBytes();
        std::copy(Peek(), Peek() + readable, BeginPtr());
        readPos_ = 0;
        writePos_ = readable;
        assert(ReadableBytes() == readable);
    }
}
