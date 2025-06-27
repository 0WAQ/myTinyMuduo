/**
 * 
 * Logger头文件
 * 
 */
#ifndef LOGGER_H
#define LOGGER_H

#include <atomic>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <stdarg.h> // vastart va_end
#include <sys/stat.h> // mkdir
#include <cassert>

#include "base/AsyncLogging.h"
#include "base/noncopyable.h"

namespace mymuduo {

class Logger : noncopyable
{
public:
    using OutputFunc = std::function<void(const char*, size_t)>;

    // 定义日志级别
    enum LogLevel {
        DEBUG,      // 调试信息
        INFO,       // 普通信息
        WARN,       // 警告信息
        ERROR,      // 报错
    };

public:
    static Logger* instance();

    void set_log_level(LogLevel level);
    bool set_output(OutputFunc func);
    bool set_async(const std::shared_ptr<AsyncLogging>& async);

    void append_with_level_title(LogLevel level, std::string& msg);
    void write(LogLevel level, const char* format, ...);

    const LogLevel log_level() const noexcept { return _M_level; };
    const bool initialized() const noexcept { return _M_initialized; }

private:
    Logger();
    Logger(const std::filesystem::path& path, const std::string& basename, int roll_size);

private:
    std::atomic<bool> _M_initialized;

    // 日志等级
    LogLevel _M_level;

    // 日志输出函数
    OutputFunc _M_output_func;
};

} // namespace mymuduo


////////////////////////////////////////////////////////////////////////////////////

#define LOG_BASE(level, format, ...)                                \
    do {                                                            \
        mymuduo::Logger* log = mymuduo::Logger::instance();         \
        if(log->initialized())                                      \
            log->write(mymuduo::level, format, ##__VA_ARGS__);      \
    } while(0)


#ifndef RELEASE
    #define LOG_DEBUG(format, ...) LOG_BASE(Logger::DEBUG, format, ##__VA_ARGS__)
#else
    #define LOG_DEBUG(format, ...)
#endif

#define LOG_INFO(format, ...)  LOG_BASE(Logger::INFO, format, ##__VA_ARGS__)
#define LOG_WARN(format, ...)  LOG_BASE(Logger::WARN, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) \
            do { \
                LOG_BASE(Logger::ERROR, format, ##__VA_ARGS__); exit(-1); \
            } while(0)

////////////////////////////////////////////////////////////////////////////////////

#endif // LOGGER_H
