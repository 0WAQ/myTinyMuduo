#pragma once

#include <iostream>
#include <string>
#include <cstdint>
#include <cstring>
#include <vector>
#include <unistd.h>
#include <sys/uio.h>
#include <cassert>

class Buffer
{
public:
    
    /// @brief 初始化buffer
    /// @param sep 选择报文分割类型: 0-无分割符; 1-四字节的报文头; 2-"\r\n\r\n"分割符(Http协议)
    Buffer(uint16_t sep = 1, std::size_t prependable_size = 8, std::size_t writable_size = 1024);


    /// @brief 将字符串追加到缓冲区, 带有分割符
    /// @param msg 
    void append(const std::string& msg);


    /// @brief 将数据追加至缓冲区
    /// @param data 数据首地址
    /// @param size 数据大小
    void append(const char* data, size_t size);
    

    /// @brief 从buf中取出一个报文
    /// @param msg 报文数据
    /// @return 成功true, 失败false
    bool pick_datagram(std::string& msg);


    /// @brief 删除size的大小的数据, 并返回
    /// @param size 
    /// @return 
    std::string erase(std::size_t size);


    /// @brief 扩容
    /// @param len 扩容到len 
    void resize(std::size_t len);


    /// @brief 确保缓冲区有size的大小
    /// @param size 
    void ensure_writable(std::size_t size);


    /// @brief 写区域增加len的大小
    /// @param size 
    void has_written(std::size_t size) { _M_write_idx += size; }


    /// @brief 获取缓冲区的首地址
    char* begin() { return &*_M_buf.begin(); }
    const char* cbegin() const { return &*_M_buf.begin(); }


    /// @brief 获取缓冲区三个区域的大小
    /// @return 大小 
    std::size_t prependable() { return _M_read_idx; }
    std::size_t readable()    { return _M_write_idx - _M_read_idx; }
    std::size_t writable()    { return _M_buf.size() - _M_write_idx; }


    /// @brief 直接操作内核缓冲区和用户缓冲区
    /// @param fd 
    /// @return 
    std::size_t read_fd(int fd);
    std::size_t write_fd(int fd);


private:

    // prependable的初始大小, 也是read和write idx的初始值
    std::size_t _M_initial_prependable;

    // writable的初始大小
    std::size_t _M_initial_writable;

    // read和write区域的索引
    std::size_t _M_read_idx;
    std::size_t _M_write_idx;

    // 自动扩容的vector<char>
    std::vector<char> _M_buf;

    // 分割符
    const uint16_t _M_sep;
    
};