#include "../include/Buffer.hpp"

Buffer::Buffer() {

}

void Buffer::append(const char* data, size_t size)
{
    _M_buf.append(data, size);
}

std::size_t Buffer::get_buf_size()
{
    return _M_buf.size();
}

const char* Buffer::get_buf_data()
{
    return _M_buf.data();
}

void Buffer::clear()
{
    _M_buf.clear();
}

Buffer::~Buffer() {

}