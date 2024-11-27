/**
 * 
 * Logger头文件
 * 
 */
#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <string>
#include <mutex>
#include <memory>
#include <stdarg.h> // vastart va_end
#include <sys/stat.h> // mkdir
#include <cassert>
#include "Buffer.h"
#include "TimeStamp.h"
#include "ThreadPool.h"

// 定义日志级别
enum LogLevel
{
    DEBUG,      // 调试信息
    INFO,       // 普通信息
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


    /// @brief 初始化文件, 服务器启动时应该调用的方法
    /// @param level 
    /// @param path 文件路径, 最后一个目录不需要/
    /// @param suffix 后缀名
    void init(LogLevel level, std::string path, std::string suffix);


    /// @brief 设置日志等级
    /// @param level 
    void set_level(LogLevel level);


    /// @brief 获取当前日志等级
    /// @return 
    LogLevel get_level();


    /// @brief 添加日志等级头到缓冲区
    /// @param level
    /// @param msg
    void append_level_title(LogLevel level, std::string& msg);


    /// @brief 将日志信息写入缓冲区的任务添加至LOG线程池
    /// @param level
    /// @param format
    void write(LogLevel level, const char* format, ...);


    /// @brief 把日志信息写入缓冲区
    /// @param msg 
    void write_async(std::string msg);


    /// @brief 文件是否打开
    /// @return true-打开, false-关闭
    bool is_open() { return _M_is_open; }


private:

    /// @brief 将构造与析构设置为private, 禁止创建单例对象
    Logger();
    ~Logger();


private:

    /**
     * Logger类
     */

    // 日志等级
    LogLevel _M_level;

    // 缓冲区
    Buffer _M_buffer;


    /**
     * 文件操作
     */

    // 文件路径名
    std::string _M_dir_name;

    // 文件后缀名
    std::string _M_suffix_name;

    // 记录今天的日期
    std::string _M_today;

    // 文件指针
    FILE* _M_fp;

    // 最大行数
    static const size_t _M_max_lines = 50000;

    // 记录行数
    size_t _M_line_cnt;

    // 文件是否打开
    bool _M_is_open;


    /**
     * 多线程相关
     */

    // 线程池
    size_t _M_thread_num;
    std::unique_ptr<ThreadPool> _M_pool;

    // 锁
    std::mutex _M_mutex;
};


////////////////////////////////////////////////////////////////////////////////////
#define LOG_BASE(level, format, ...)                            \
    do {                                                        \
        Logger* log = Logger::get_instance();                   \
        if(log->is_open() && log->get_level() <= level) {       \
            log->write(level, format, ##__VA_ARGS__);           \
        }                                                       \
    } while(0)

#define LOG_DEBUG(format, ...) LOG_BASE(DEBUG, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...)  LOG_BASE(INFO,  format, ##__VA_ARGS__)
#define LOG_WARN(format, ...)  LOG_BASE(WARN,  format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) LOG_BASE(ERROR, format, ##__VA_ARGS__)
////////////////////////////////////////////////////////////////////////////////////

#endif // LOGGER_H