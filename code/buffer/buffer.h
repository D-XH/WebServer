#pragma once

#include <unistd.h>
#include <vector>
#include <atomic>
#include <string>
#include <string.h>
#include <sys/uio.h>
#include <assert.h>


class Buffer {
public:
    Buffer(int initialBufferSize = 1024);
    ~Buffer() = default;

    // 状态信息
    size_t WritableBytes() const;
    size_t ReadableBytes() const;
    size_t PrependableBytes() const;

    // 移动下标操作
    const char* Peek() const;
    void EnsureWritable(size_t len);
    void HasWritten(size_t len);

    void Retrieve(size_t len);
    void RetrieveUntil(const char* end);

    void RetrieveAll();
    std::string RetrieveAllToStr();

    const char* BeginWriteConst() const;
    char* BeginWrite();

    // 写入操作
    void Append(const std::string& str);
    void Append(const char* str, size_t len);
    void Append(const void* data, size_t len);
    void Append(const Buffer& buff);

    // 
    ssize_t ReadFd(int Fd, int* ErrNo);
    ssize_t WriteFd(int Fd, int* ErrNO);

private:
    char* BeginPtr();
    void MakeSpace_(size_t len);

private:
    std::vector<char> buffer_;
    std::atomic<size_t> readPos_;
    std::atomic<size_t> writePos_;
};