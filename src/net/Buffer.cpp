#include "net/Buffer.h"

using namespace mymuduo;
using namespace mymuduo::net;

Buffer::Buffer(std::size_t prependable_size, std::size_t writable_size) : 
            _M_initial_prependable(prependable_size), _M_initial_writable(writable_size), 
            _M_read_idx(_M_initial_prependable), _M_write_idx(_M_initial_prependable),
            _M_buf(_M_initial_prependable + _M_initial_writable)
{ }

Buffer::Buffer(const Buffer& other)
    : _M_initial_prependable(other._M_initial_prependable)
    , _M_initial_writable(other._M_initial_writable)
    , _M_read_idx(other._M_read_idx)
    , _M_write_idx(other._M_write_idx)
    , _M_buf(other._M_buf)
    , _M_sep(other._M_sep)
{ }

Buffer::Buffer(Buffer&& other) noexcept
    : _M_initial_prependable(other._M_initial_prependable)
    , _M_initial_writable(other._M_initial_writable)
    , _M_read_idx(other._M_read_idx)
    , _M_write_idx(other._M_write_idx)
    , _M_buf(std::move(other._M_buf))
    , _M_sep(other._M_sep)
{
    other._M_initial_prependable = other._M_initial_writable = 0;
    other._M_read_idx = other._M_write_idx = 0;
    other._M_sep = None;
}

Buffer& Buffer::operator= (const Buffer& other) {
    _M_initial_prependable = other._M_initial_prependable;
    _M_initial_writable = other._M_initial_writable;
    _M_read_idx = other._M_read_idx;
    _M_write_idx = other._M_write_idx;
    _M_buf = other._M_buf;
    _M_sep = other._M_sep;
    return *this;
}

Buffer& Buffer::operator= (Buffer&& other) {
    _M_initial_prependable = other._M_initial_prependable;
    _M_initial_writable = other._M_initial_writable;
    _M_read_idx = other._M_read_idx;
    _M_write_idx = other._M_write_idx;
    _M_buf = std::move(other._M_buf);
    _M_sep = other._M_sep;
    return *this;
}

void Buffer::append_with_sep(const char* data, std::size_t size)
{
    switch (_M_sep)
    {
        case 0: // 无分割符
        {
            append(data, size);
            break;
        }
        case 1: // 固定长度4字节做报文头
        {
            // 先将数据的大小送入缓冲区
            append((char*)&size, 4);    
            // 再将数据本身送入缓冲区
            append(data, size);
            break;
        }
        case 2: // 报文尾部加 "\r\n\r\n"
        {
            // 先将数据放入缓冲区
            append(data, size);
            // 再放入分割符
            append("\r\n\r\n", 4);

            break;
        }
    }
}

void Buffer::append_with_sep(const std::string& msg) {
    append_with_sep(msg.data(), msg.size());
}

void Buffer::append(const char* data, std::size_t size)
{
    assert(data != nullptr);
    ensure_writable(size);
    std::copy(data, data + size, begin() + _M_write_idx);
    has_written(size);
}

void Buffer::append(const std::string& msg) {
    append(msg.data(), msg.size());
}

std::string Buffer::erase(std::size_t size)
{
    // 不会有这种情况出现
    if(size > readable()) {
        return {};
    }

    std::string res(begin() + _M_read_idx, size);

    // 将索引向后移动
    _M_read_idx += size;

    // 若移动后, payload大小为0, 则重置索引
    if(readable() == 0) {
        _M_read_idx = _M_write_idx = _M_initial_prependable;
    }

    return res;
}

bool Buffer::pick_datagram(std::string& msg)
{
    // 若可读区域大小为0, 失败
    if(readable() == 0)
        return false;

    switch (_M_sep)
    {
        case 0:
        {
            msg = std::string{begin() + _M_read_idx, begin() + _M_write_idx};
            erase(readable());
            break;
        }
        case 1:
        {
            std::size_t len = 0; // 获取报文头部的长度
            memcpy(&len, _M_buf.data() + _M_read_idx, 4);

            // 若payload的长度小于报文(有分包现象), 则不完整, 等待
            if(readable() < len + 4)
                return false;

            // 跳过报文头, 只取报文内容
            msg = {begin() + _M_read_idx + 4, len};
            erase(len + 4); // 将该报文从缓冲区中删除

            break;
        }
        case 2:
        {
            std::size_t len = std::string::npos;
            for(std::size_t i = _M_read_idx; i + 3 < _M_write_idx; i++) {
                if(std::string{begin() + i, begin() + i + 4} == "\r\n\r\n") {
                    len = i;
                    break;
                }
            }

            // 没有找到分割符 或者 报文不完整
            if(len == std::string::npos || readable() < len - _M_read_idx + 4)
                return false;

            // 将报文放到msg中
            msg = {begin() + _M_read_idx, begin() + len};
            erase(len - _M_read_idx + 4);

            break;
        }
    }

    return true;
}

void Buffer::ensure_writable(std::size_t size)
{
    // 确保可写区域有size的大小, 否则扩容
    if(writable() < size) {
        resize(size);
    }
}

void Buffer::has_written(std::size_t size) {
    _M_write_idx += size;
}

void Buffer::resize(std::size_t len)
{
    // 当writable和读空下来的prependable不够时, 重新分配
    if(writable() + prependable() < len + _M_initial_prependable) 
    {
        // 重新分配空间
        _M_buf.resize(_M_write_idx + len);
    }
    else // 空下来的大小足够分配
    {
        // 提前获取payload的大小
        std::size_t readable_bytes = readable();
        
        // 将payload移动到初始位置
        std::copy(begin() + _M_read_idx, begin() + _M_write_idx, 
                  begin() + _M_initial_prependable);

        // 更新两个索引的位置        
        _M_read_idx = _M_initial_prependable;
        _M_write_idx = _M_read_idx + readable_bytes;
    }
}

std::size_t Buffer::read_fd(int fd, int* save_errno)
{
    // MARK: 一开始不知道input缓冲区是否会因为空间不足而溢出
    //       故开辟栈上缓冲, 扩容后再将其拷贝给input缓冲

    char extrabuf[65536] = {0};  // 临时缓冲区

    struct iovec iov[2];
    const std::size_t write_bytes = writable();

    // 利用readv分散读, 会优先将缓冲区写满, 剩余的会写到buf中
    iov[0].iov_base = begin() + _M_write_idx;
    iov[0].iov_len = write_bytes;

    iov[1].iov_base = extrabuf;
    iov[1].iov_len = sizeof(extrabuf);

    // 如果 buffer 的可写区域大于 extrabuf, 那么忽略 extrabuf
    const int iov_count = (write_bytes < sizeof(extrabuf)) ? 2 : 1;

    ssize_t nlen = ::readv(fd, iov, iov_count);
    // error
    if(nlen < 0) {
        *save_errno = errno;
    }
    // 只会读到缓冲区中
    else if(nlen <= write_bytes) 
    {
        _M_write_idx += nlen;
    }
    // 还会读到extrabuf中
    else 
    {
        _M_write_idx = _M_buf.size();
        // 将extrabuf中的数据追加至缓冲区中(会触发resize)
        append_with_sep(extrabuf, nlen - write_bytes);
    }
    return nlen;
}

std::size_t Buffer::write_fd(int fd, int* save_errno)
{
    // 将缓冲区的payload全部发送出去
    std::size_t read_size = readable();
    ssize_t len = ::write(fd, begin() + _M_read_idx, read_size);

    if(len < 0) {
        *save_errno = errno;
    } 
    // 调整索引
    else {
        _M_read_idx += len;
    }

    return len;
}

void Buffer::retrieve(size_t len)
{
    if(len < readable()) {
        _M_read_idx += len;
    }
    else {
        retrieve_all();
    }
}

void Buffer::retrieve_all() {
    _M_read_idx = _M_write_idx = _M_initial_prependable;
}

std::string Buffer::retrieve_as_string(size_t len) {
    std::string res(begin() + _M_read_idx, len);
    retrieve(len);
    return res;
}

std::string Buffer::retrieve_all_as_string() {
    return retrieve_as_string(readable());
}

