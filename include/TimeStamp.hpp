#pragma once

#include <iostream>
#include <string>


/**
 * 
 * 时间戳类
 * 
 */
class TimeStamp
{
public:

    /**
     * 
     * @describe: 用当前时间初始化对象
     * @param:    void
     * 
     */
    TimeStamp();


    /**
     * 
     * @describe: 用指定时间初始化对象
     * @param:    int64_t
     * 
     */
    TimeStamp(int64_t sec);


    /**
     * 
     * @describe: 获取当前时间对象
     * @param:    void
     * @return:   static TimeStamp
     * 
     */
    static TimeStamp now();


    /**
     * 
     * @describe: 返回当前对象的整数表示时间
     * @param:    void
     * @return:   time_t
     * 
     */
    time_t to_time_t() const;


    /**
     * 
     * @describe: 返回当前对象的字符串表示时间
     * @param:    void
     * @return:   std::string, 格式为 yyyy-mm-dd hh24:mi:ss
     */
    std::string to_string() const;


    ~TimeStamp();

private:
    time_t _M_sec; // 从1970到现在
};