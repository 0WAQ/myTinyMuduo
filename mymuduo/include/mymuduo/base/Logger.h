#ifndef MYMUDUO_BASE_LOGGER_H
#define MYMUDUO_BASE_LOGGER_H

#include <atomic>
#include <functional>
#include <memory>
#include <string>

#include "mymuduo/base/AsyncLogging.h"
#include "mymuduo/base/noncopyable.h"
#include "mymuduo/base/singleton.h"

namespace mymuduo {

class Logger : noncopyable
             , public Singleton<Logger>
{
public:
    friend class Singleton<Logger>;

    using OutputFunc = std::function<void(const char*, size_t)>;

    // 定义日志级别
    enum LogLevel {
        DEBUG,      // 调试信息
        INFO,       // 普通信息
        WARN,       // 警告信息
        ERROR,      // 报错
        NUM_LOG_LEVELS
    };

public:
    void set_log_level(LogLevel level);
    bool set_output(OutputFunc func);
    bool set_async(const std::shared_ptr<AsyncLogging>& async);

    void append_with_level_title(LogLevel level, std::string& msg);
    void write(LogLevel level, const char* format, ...);

    const LogLevel log_level() const noexcept { return _level; };
    const bool initialized() const noexcept { return _initialized; }

private:
    std::atomic<bool> _initialized { false };

    // 日志等级
    LogLevel _level = INFO;

    // 日志输出函数
    OutputFunc _output_func;
};

} // namespace mymuduo


////////////////////////////////////////////////////////////////////////////////////

#define LOG_BASE(level, format, ...)                                \
    do {                                                            \
        using namespace mymuduo;                                    \
        if(Logger::instance().initialized())                                       \
            Logger::instance().write(mymuduo::level, format, ##__VA_ARGS__);       \
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

#endif // MYMUDUO_BASE_LOGGER_H
