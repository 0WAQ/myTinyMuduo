#include "../include/Buffer.hpp"

Buffer::Buffer(uint16_t sep) : _M_sep(sep) { }

void Buffer::append(const char* data, size_t size) { _M_buf.append(data, size);}

void Buffer::append_with_sep(const char* data, size_t size)
{
    switch (_M_sep)
    {
    case 0:
        _M_buf.append(data, size);
        break;

    case 1:

        // 先将数据的大小送入缓冲区
        _M_buf.append((char*)&size, 4);
        // 再将数据本身送入缓冲区
        _M_buf.append(data, size);
        break;

    case 2:
    {
        // 先将数据放入缓冲区
        _M_buf.append(data, size);
        // 再放入分割符
        std::string sep{"\r\n\r\n"};
        _M_buf.append(sep.data(), sep.size());
        break;
    }
    }

}

bool Buffer::pick_datagram(std::string& ss)
{
    if(_M_buf.empty())  
        return false;

    switch (_M_sep)
    {
    case 0:
    {        
        ss = _M_buf;
        _M_buf.clear();

        break;
    }
    case 1:
    {
        int len; // 获取报文头部
        memcpy(&len, _M_buf.data(), 4); // 从buf中获取报头
        
        // 若接收缓冲区中的数据量长度小于报文(有分包现象), 则不完整, 等待
        if(_M_buf.size() < len + 4)
            return false;

        // 跳过报文头, 只取报文内容
        ss.append(_M_buf.data() + 4, len);
        _M_buf.erase(0, len + 4); // 将该报文从缓冲区中删除

        break;
    }
    case 2:
    {
        int len = _M_buf.find("\r\n\r\n");

        // 没有找到分割符 或者 报文不完整
        if(len == std::string::npos || _M_buf.size() < len + 4)
            return false;

        ss.append(_M_buf.data(), len);
        _M_buf.erase(0, len + 4);

        break;
    }
    }

    return true;
}

void Buffer::erase(size_t pos, size_t len) { _M_buf.erase(pos, len);}

std::size_t Buffer::size() { return _M_buf.size();}

const char* Buffer::data() { return _M_buf.data();}

void Buffer::clear() { _M_buf.clear();}

Buffer::~Buffer() { }