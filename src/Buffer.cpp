#include "../include/Buffer.hpp"

Buffer::Buffer() {

}

void Buffer::append(const char* data, size_t size)
{
    _M_buf.append(data, size);
}

void Buffer::append_with_head(const char* data, size_t size)
{
    // 先将数据的大小送入缓冲区
    _M_buf.append((char*)&size, 4);
    // 再将数据本身送入缓冲区
    _M_buf.append(data, size);
}

void Buffer::erase(size_t pos, size_t len)
{
    _M_buf.erase(pos, len);
}

std::size_t Buffer::size()
{
    return _M_buf.size();
}

const char* Buffer::data()
{
    return _M_buf.data();
}

void Buffer::clear()
{
    _M_buf.clear();
}

Buffer::~Buffer() {

}