#pragma once

#include <iostream>
#include <string>
#include <mutex>
#include <stdarg.h> // vastart va_end
#include "Buffer.hpp"
#include "TimeStamp.hpp"
#include "ThreadPool.hpp"

// 定义日志级别
enum LogLevel
{
    INFO,       // 普通信息
    DEBUG,      // 调试信息
    WARN,       // 警告信息
    ERROR       // 报错
};

class ThreadPool;

/// @brief 日志类, 单例模式
class Logger
{
public:

    /// @brief 单例模式, 获取唯一实例(懒汉模式)
    /// @return 单例对象
    static Logger* get_instance();


    /// @brief 设置日志等级
    /// @param level 
    void set_log_level(LogLevel level);


    /// @brief 添加日志等级头到缓冲区
    void append_level_title();


    /// @brief 将日志信息写入缓冲区的任务添加至LOG线程池
    /// @param format
    void write(const char* format, ...);


    /// @brief 把日志信息写入缓冲区
    /// @param msg 
    void write_async(std::string msg);

private:

    /// @brief 将构造与析构设置为private, 禁止创建单例对象
    Logger();
    ~Logger();


private:

    // 日志等级
    LogLevel _M_level;

    // 缓冲区
    Buffer _M_buffer;

    // 线程池
    size_t _M_thread_num;
    ThreadPool* _M_pool;

    // 锁
    std::mutex _M_mutex;
};


////////////////////////////////////////////////////////////////////////////////////
#define LOG_BASE(level, format, ...)                    \
    do {                                                \
        Logger* log = Logger::get_instance();           \
        log->set_log_level(level);                      \
        log->write(format, ##__VA_ARGS__);              \
    } while(0)

#define LOG_INFO(format, ...)  LOG_BASE(INFO,  format, ##__VA_ARGS__)
#define LOG_DEBUG(format, ...) LOG_BASE(DEBUG, format, ##__VA_ARGS__)
#define LOG_WARN(format, ...)  LOG_BASE(WARN,  format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) LOG_BASE(ERROR, format, ##__VA_ARGS__)
////////////////////////////////////////////////////////////////////////////////////