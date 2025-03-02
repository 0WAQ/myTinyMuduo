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
#include "noncopyable.h"

// 日志文件最大行数
#define LOG_MAX_LINES       (5000)

// 定义日志级别
enum LogLevel
{
    DEBUG,      // 调试信息
    INFO,       // 普通信息
    WARN,       // 警告信息
    ERROR,      // 报错
};

////////////////////////////////////////////////////////////////////////////////////

#define LOG_BASE(level, format, ...)                            \
    do {                                                        \
        Logger* log = Logger::get_instance();                   \
        if(log->is_open() && log->get_level() <= level) {       \
            log->write(level, format, ##__VA_ARGS__);           \
        }                                                       \
    } while(0)


#ifndef RELEASE
    #define LOG_DEBUG(format, ...) LOG_BASE(DEBUG, format, ##__VA_ARGS__)
#else
    #define LOG_DEBUG(format, ...)
#endif

#define LOG_INFO(format, ...)  LOG_BASE(INFO,  format, ##__VA_ARGS__)
#define LOG_WARN(format, ...)  LOG_BASE(WARN,  format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) do { LOG_BASE(ERROR, format, ##__VA_ARGS__); exit(-1); } while(0)

////////////////////////////////////////////////////////////////////////////////////

class ThreadPool;

/// @brief 日志类, 单例模式
class Logger : noncopyable
{
public:

    /**
     * @brief 单例模式, 获取唯一实例(懒汉模式)
     */
    static Logger* get_instance() {
        // 懒汉模式, 在第一次调用时才创建对象
        static Logger instance;  // c++11以后, 使用局部变量懒汉不用加锁
        return &instance;
    }

    /**
     * @brief 初始化日志系统
     * @param path 日志文件的路径, 如 /path/to/logfile
     * @param suffix 日志文件的后缀名
     */
    void init(LogLevel level, std::string path, std::string suffix);

    /**
     * @brief 设置日志等级
     */
    void set_level(LogLevel level);

    /**
     * @brief 获取日志等级
     */
    LogLevel get_level();
    
    /**
     * @brief 判断日志文件是否打开
     */
    bool is_open();

    /**
     * @brief 添加日志等级头到缓冲区
     */
    void append_level_title(LogLevel level, std::string& msg);

    /**
     * @brief 将日志消息交给异步线程去写入
     */
    void write(LogLevel level, const char* format, ...);

    /**
     * @brief 把日志信息写入缓冲区
     */
    void write_async(std::string& msg);


private:

    /**
     * @brief 将构造与析构设置为private, 禁止创建与销毁单例对象
     */
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
     * 日志文件相关操作
     */

        // 日志文件路径名
        std::string _M_dir_name;

        // 日志文件后缀名
        std::string _M_suffix_name;

        // 记录今天的日期
        std::string _M_today;

        // 日志文件指针
        FILE* _M_fp;

        // 文件是否打开, 主要是用于防止线程池任务在log->init前打印
        bool _M_is_open = false;

        // 记录日志文件最大行数
        const size_t _M_max_lines = LOG_MAX_LINES;

        // 记录日志当前行数
        size_t _M_line_cnt;


    /**
     * 异步相关
     */

        // 线程池
        size_t _M_thread_num;
        std::unique_ptr<ThreadPool> _M_pool;

        // 锁
        std::mutex _M_mutex;
};

#endif // LOGGER_H