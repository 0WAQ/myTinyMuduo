#ifndef MYMUDUO_BASE_LOGGER_H
#define MYMUDUO_BASE_LOGGER_H

#include "mymuduo/base/LogFormat.h"
#include "mymuduo/base/Timestamp.h"

#include <functional>

namespace mymuduo {

class Logger
{
public:
    using OutputFunc = std::function<void(const char*, size_t len)>;
    using FlushFunc = std::function<void()>;

    // 定义日志级别
    enum LogLevel {
        DEBUG,      // 调试信息
        INFO,       // 普通信息
        WARN,       // 警告信息
        ERROR,      // 报错
        UNKN,       // 未知
        NUM_LOG_LEVELS
    };

public:
    Logger(LogLevel level);
    ~Logger();

    template <typename... Args>
    void format(std::format_string<Args...> fmt, Args... args) {
        _impl._format.format(fmt, std::forward<Args>(args)...);
    }

    static LogLevel log_level();
    static void set_log_level(LogLevel level);
    static void set_output(OutputFunc func);
    static void set_flush(FlushFunc func);

private:

    class Impl {
    public:
        Impl(LogLevel level);

        void format_time();
        void finish();

        Timestamp _ts;
        LogFormat _format;
        LogLevel _level;
    };

    Impl _impl;
};

} // namespace mymuduo


////////////////////////////////////////////////////////////////////////////////////

#define LOG_BASE(level, fmt, ...)                                                       \
    do {                                                                                \
        using namespace mymuduo;                                                        \
        if (Logger::log_level() <= level)                                               \
            Logger(level).format(fmt, ##__VA_ARGS__);                                   \
    } while(0)


#ifndef RELEASE
    #define LOG_DEBUG(fmt, ...) LOG_BASE(Logger::DEBUG, fmt, ##__VA_ARGS__)
#else
    #define LOG_DEBUG(fmt, ...)
#endif

#define LOG_INFO(fmt, ...)  LOG_BASE(Logger::INFO, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  LOG_BASE(Logger::WARN, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) LOG_BASE(Logger::ERROR, fmt, ##__VA_ARGS__)

////////////////////////////////////////////////////////////////////////////////////

#endif // MYMUDUO_BASE_LOGGER_H
