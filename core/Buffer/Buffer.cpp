#include "Buffer.h"

Buffer::Buffer(int buf_capacity)
:_buf(buf_capacity)
{
    _read_pos = 0;
    _write_pos = 0;
}

int Buffer::Read(int fd, int *Errno)
{
    char buff[65536] = {0};
    iovec iov[2];
    iov[0].iov_base = WriteBeginPtr();
    iov[0].iov_len = WriteableBytes();
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    ssize_t r_cnt = readv(fd, iov, 2);
    if(r_cnt < 0){
        // log_err
        *Errno = errno;
    }else if(r_cnt < iov[0].iov_len){
        ForwardWritePos(r_cnt);
    }else{
        _write_pos = _buf.size();
        Append(buff, r_cnt-iov[0].iov_len);
    }

    return r_cnt;
}

int Buffer::Write(int fd, int *Errno)
{
    ssize_t w_cnt = write(fd, ReadBeginPtr(), ReadableBytes());
    if(w_cnt < 0){
        // log_err
        *Errno = errno;
    }else{
        Retrive(w_cnt);
    }
    return w_cnt;
}

void Buffer::Append(const char *data, size_t len)
{
    assert(data);
    EnsureWriteable(len);
    std::copy(data, data+len, WriteBeginPtr());
    ForwardWritePos(len);
}

void Buffer::Append(const string &str)
{
    Append(str.data(), str.size());
}

size_t Buffer::ReadableBytes() const
{
    return _write_pos - _read_pos;
}

size_t Buffer::WriteableBytes() const
{
    return _buf.size() - _write_pos;
}

size_t Buffer::ReadedBytes() const
{
    return _read_pos;
}

void Buffer::Retrive(size_t n)
{
    assert(n <= ReadableBytes());
    _read_pos += n;
}

void Buffer::RetriveUntil(const char *end)
{
    assert(ReadBeginPtr() <= end);
    Retrive(end - ReadBeginPtr());
}

void Buffer::RetriveAll()
{
    bzero(_begin_ptr(), _buf.size());
    _read_pos = _write_pos = 0;
}

std::string Buffer::RetriveAllToStr()
{
    std::string str(ReadBeginPtr(), ReadableBytes());
    RetriveAll();
    return str;
}

char *Buffer::ReadBeginPtr()
{
    return _begin_ptr() + _read_pos;
}

char *Buffer::WriteBeginPtr()
{
    return _begin_ptr() + _write_pos;
}

void Buffer::ForwardWritePos(size_t n)
{
    _write_pos += n;
}

void Buffer::EnsureWriteable(size_t n)
{
    if(WriteableBytes() < n){
        _makeSpace(n);
    }
    assert(n < WriteableBytes());
}

char *Buffer::_begin_ptr()
{
    return &*_buf.begin();
}

void Buffer::_makeSpace(size_t n)
{
    std::copy(_buf.begin()+_read_pos, _buf.begin()+_write_pos, _buf.begin());
    _write_pos -= _read_pos;
    _read_pos = 0;
    if(WriteableBytes() < n){
        _buf.resize(_write_pos + n + 1);
    }
}
