#pragma once

#include <iostream>
#include <string>
#include <cstdint>
#include <cstring>

class Buffer
{
public:
    
    /// @brief 初始化buffer
    /// @param sep 选择报文分割类型: 0-无分割符; 1-四字节的报文头; 2-"\r\n\r\n"分割符(Http协议)
    Buffer(uint16_t sep = 1);


    /// @brief 将数据追加至缓冲区
    /// @param data 数据首地址
    /// @param size 数据大小
    void append(const char* data, size_t size);
    

    /// @brief 将带有报头的数据追加至buf
    /// @param data 同上
    /// @param size 同上
    void append_with_sep(const char* data, size_t size);


    /// @brief 从buf中取出一个报文
    /// @param msg 报文数据
    /// @return 成功true, 失败false
    bool pick_datagram(std::string& msg);


    /// @brief 从buf中删除数据
    /// @param pos 起始位置 
    /// @param len 长度
    void erase(size_t pos, size_t len);


    /// @brief 获取buf的长度
    /// @return 
    size_t size();


    /// @brief 获取数据首地址
    /// @return 
    const char* data();
    
    
    /// @brief 清空buf
    void clear();


    /// @brief 
    ~Buffer();

private:

    std::string _M_buf;

    const uint16_t _M_sep;
    
};