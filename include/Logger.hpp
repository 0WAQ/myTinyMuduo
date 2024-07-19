#pragma once

#include <iostream>
#include <string>
#include <stdarg.h> // vastart va_end
#include "Buffer.hpp"
#include "TimeStamp.hpp"

// 定义日志级别
enum LogLevel
{
    INFO,       // 普通信息
    DEBUG,      // 调试信息
    WARN,       // 警告信息
    ERROR       // 报错
};

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


    /// @brief 将日志信息写入缓冲区
    /// @param format
    void write(const char* format, ...);


    /// @brief 刷新缓冲区
    void flush();


private:

    /// @brief 将构造与析构设置为private, 禁止创建单例对象
    Logger();
    ~Logger();


private:
    LogLevel _M_level;

    Buffer _M_buffer;
};


////////////////////////////////////////////////////////////////////////////////////
#define LOG_BASE(level, format, ...)                    \
    do {                                                \
        Logger* log = Logger::get_instance();           \
        log->set_log_level(level);                      \
        log->append_level_title();                      \
        log->write(format, ##__VA_ARGS__);              \
        log->flush();                                   \
    } while(0)

#define LOG_INFO(format, ...)  LOG_BASE(INFO,  format, ##__VA_ARGS__)
#define LOG_DEBUG(format, ...) LOG_BASE(DEBUG, format, ##__VA_ARGS__)
#define LOG_WARN(format, ...)  LOG_BASE(WARN,  format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) LOG_BASE(ERROR, format, ##__VA_ARGS__)
////////////////////////////////////////////////////////////////////////////////////