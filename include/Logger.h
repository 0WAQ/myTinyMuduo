/**
 * 
 * Logger头文件
 * 
 */
#ifndef LOGGER_H
#define LOGGER_H

#include <filesystem>
#include <string>
#include <stdarg.h> // vastart va_end
#include <sys/stat.h> // mkdir
#include <cassert>
#include "AsyncLogging.h"
#include "noncopyable.h"

namespace mymuduo
{

// TODO: 增加新功能: 可以重定向日志的输出到任意位置

/// @brief 日志类, 单例模式
class Logger : noncopyable
{
public:

    // 定义日志级别
    enum LogLevel
    {
        DEBUG,      // 调试信息
        INFO,       // 普通信息
        WARN,       // 警告信息
        ERROR,      // 报错
    };

public:

    /**
     * @brief 单例模式, 获取唯一实例(懒汉模式)
     */
    static Logger* get_instance(const std::string& path = "stdout", const std::string& basename = "Server", int roll_size = 500*1000*1000) {
        // 懒汉模式, 在第一次调用时才创建对象
        static Logger log(path, basename, roll_size);   // c++11以后, 使用局部变量懒汉不用加锁
        return &log;
    }

    /**
     * @brief 初始化日志系统
     */
    void init(LogLevel level);

    /**
     * @brief 设置日志等级
     */
    void set_log_level(LogLevel level);

    /**
     * @brief 获取日志等级
     */
    LogLevel log_level();

    /**
     * @brief 添加日志等级头到缓冲区
     */
    void append_level_title(LogLevel level, std::string& msg);

    /**
     * @brief 将日志消息交给异步线程去写入
     */
    void write(LogLevel level, const char* format, ...);

private:

    /**
     * @brief 将构造与析构设置为private, 禁止创建与销毁单例对象
     */
    Logger();
    Logger(const std::filesystem::path& path, const std::string& basename, int roll_size);

private:

    // 日志等级
    LogLevel _M_level;

    // 异步日志系统
    AsyncLogging _M_async_logging;

};

} // namespace mymuduo


////////////////////////////////////////////////////////////////////////////////////

#define LOG_BASE(level, format, ...)                                \
    do {                                                            \
        mymuduo::Logger* log = mymuduo::Logger::get_instance();     \
        if(log->log_level() <= mymuduo::level) {                    \
            log->write(mymuduo::level, format, ##__VA_ARGS__);      \
        }                                                           \
    } while(0)


#ifndef RELEASE
    #define LOG_DEBUG(format, ...) LOG_BASE(Logger::DEBUG, format, ##__VA_ARGS__)
#else
    #define LOG_DEBUG(format, ...)
#endif

#define LOG_INFO(format, ...)  LOG_BASE(Logger::INFO, format, ##__VA_ARGS__)
#define LOG_WARN(format, ...)  LOG_BASE(Logger::WARN, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) do { LOG_BASE(Logger::ERROR, format, ##__VA_ARGS__); exit(-1); } while(0)

////////////////////////////////////////////////////////////////////////////////////

#endif // LOGGER_H
