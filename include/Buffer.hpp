#pragma once

#include <iostream>
#include <string>
#include <cstdint>
#include <cstring>

class Buffer
{
public:
    
    /**
     * 
     * @describe: 选择报文分割格式
     * @param:    uint16_t
     * 
     */
    Buffer(uint16_t sep = 1);

    /**
     * 
     * @describe: 将数据追加到_M_buf中
     * @param:    const char*, size_t
     * @reutrn:   void
     * 
     */
    void append(const char* data, size_t size);
    

    /**
     * 
     * @describe: 将数据追加到_M_buf中, 但是会增加头部
     * @param:    const char*. size_t
     * @return: void
     * 
     */
    void append_with_sep(const char* data, size_t size);


    /**
     * 
     * @describe: 从缓冲区中拆分出一个报文存放在ss中, 失败返回false
     * @param:    std::string
     * @return:   bool
     * 
     */
    bool pick_datagram(std::string& ss);


    /**
     * @describe: 将数据从_M_buf中删除
     * @param:    size_t, size_t
     * @return:   void
     * 
     */
    void erase(size_t pos, size_t len);


    /**
     * 
     * @describe: 获取buf的长度
     * @param:    void
     * @return:   size_t
     */
    size_t size();


    /**
     * 
     * @describe: 获取_M_buf数据的首地址
     * @param:    void
     * @return:   const char*
     * 
     */
    const char* data();
    
    
    /**
     * 
     * @describe: 清空_M_buf
     * @param:    void
     * @return:   void
     * 
     */
    void clear();


    ~Buffer();

private:
    std::string _M_buf; // 用于存放数据

    /// 0-无分割符(固定的报文长, 视频会议); 1-四字节的报文头; 2-"\r\n\r\n"分割符(Http协议)
    const uint16_t _M_sep; // 表示报文的分割符
    
};