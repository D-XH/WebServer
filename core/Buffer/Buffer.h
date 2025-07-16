#ifndef __BUFFER_H__
#define __BUFFER_H__

#include<memory>
#include<vector>
#include<atomic>
#include<string>
#include<unistd.h>
#include<string.h>
#include<sys/uio.h>
#include<assert.h>
using std::vector;
using std::atomic;
using std::string;

struct Buffer
{
public:
    explicit Buffer(int buf_capacity = 1024);
    ~Buffer() = default;

    int Read(int fd, int* Errno);   // 从fd中把数据读进缓冲区
    int Write(int fd, int* Errno);  // 从缓冲区中把数据写入fd

    void Append(const char* data, size_t len);  // 把长为len的c风格字符串写入_buf
    void Append(const string& str);             // 把string类型字符串写入_buf

    size_t ReadableBytes() const;     // 剩余可读字节。[_read_pos, _write_pos)
    size_t WriteableBytes() const;    // 剩余可写字节。[_write_pos, _buf.size())
    size_t ReadedBytes() const;       // 已读过的字节。[0, _read_pos)

    void Retrive(size_t n);                 // 回收n个字节流。前推读索引
    void RetriveUntil(const char* end);     // 回收直到end的字节流
    void RetriveAll();                      // 回收所有已读字节流
    string RetriveAllToStr();               // 回收所有已读字节流到string字符串

    char* ReadBeginPtr();   // 返回读索引所在指针
    char* WriteBeginPtr();  // 返回写索引所在指针

    void ForwardWritePos(size_t n);     // 写索引前进n个字节
    void EnsureWriteable(size_t n);     // 确保有n个字节空间可写

private:
    char* _begin_ptr();                 // 返回_buf起始指针。
    void _makeSpace(size_t n);          // 消除已读空间。如果可写字节数小于需求，申请空间。

private:
    vector<char> _buf;
    atomic<size_t> _read_pos;
    atomic<size_t> _write_pos;
};

#endif