#include "../include/Buffer.hpp"

Buffer::Buffer(uint16_t sep, std::size_t prependable_size, std::size_t writable_size) : 
            _M_sep(sep), 
            _M_initial_prependable(prependable_size), _M_initial_writable(writable_size), 
            _M_read_idx(_M_initial_prependable), _M_write_idx(_M_initial_prependable),
            _M_buf(_M_initial_prependable + _M_initial_writable)
{ }

void Buffer::append(const std::string& msg)
{
    switch (_M_sep)
    {
        case 0: // 无分割符
        {
            append(msg.data(), msg.size());
            break;
        }
        case 1: // 固定长度4字节做报文头
        {
            // 先将数据的大小送入缓冲区
            size_t len = msg.size();
            append((char*)&len, 4);    
            // 再将数据本身送入缓冲区
            append(msg.data(), msg.size());
            break;
        }
        case 2: // 报文尾部加"\r\n\r\n"
        {
            // 先将数据放入缓冲区
            append(msg.data(), msg.size());
            // 再放入分割符
            append("\r\n\r\n", 4);

            break;
        }
    }

}

void Buffer::append(const char* data, std::size_t size)
{
    assert(data != nullptr);
    ensure_writable(size);
    std::copy(data, data + size, begin() + _M_write_idx);
    has_written(size);
}

std::string Buffer::erase(std::size_t size)
{
    if(size > readable()) {
        std::cout << "删除失败" << std::endl; 
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

std::size_t Buffer::read_fd(int fd)
{

}

std::size_t Buffer::write_fd(int fd)
{
    
}