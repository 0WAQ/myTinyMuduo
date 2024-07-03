#pragma once

#include <iostream>
#include <string>

class Buffer
{
public:
    
    Buffer();

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
     * @describe: 获取buf的长度
     * @param:    void
     * @return:   size_t
     */
    size_t get_buf_size();


    /**
     * 
     * @describe: 获取_M_buf数据的首地址
     * @param:    void
     * @return:   const char*
     * 
     */
    const char* get_buf_data();
    
    
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
};