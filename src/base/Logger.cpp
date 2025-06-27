#include "base/Logger.h"
#include "base/AsyncLogging.h"
#include "base/TimeStamp.h"

#include <atomic>
#include <cstdio>
#include <functional>
#include <memory>

using namespace mymuduo;

Logger* Logger::instance() {
    // 懒汉模式, 在第一次调用时才创建对象
    static Logger logger;   // c++11以后, 使用局部变量懒汉不用加锁
    return &logger;
}

Logger::Logger()
    : _M_initialized(false)
    , _M_level(Logger::INFO)
    , _M_output_func()
{ }

void Logger::set_log_level(LogLevel level) {
    instance()->_M_level = level;
}

bool Logger::set_output(OutputFunc func) {
    if (_M_initialized.load()) {
        return false;
    }
    _M_initialized.store(true);
    _M_output_func = std::move(func);
    return true;
}

bool Logger::set_async(const std::shared_ptr<AsyncLogging>& async) {
    if (_M_initialized.load()) {
        return false;
    }
    _M_initialized.store(true);
    _M_output_func = std::bind(&AsyncLogging::append, async.get(),
                        std::placeholders::_1, std::placeholders::_2);
    async->start();
    return true;
}

void Logger::append_with_level_title(LogLevel level, std::string& msg) 
{
    switch (level) {
    case DEBUG: msg.append("[DEBUG]: "); break;
    case INFO:  msg.append("[INFO ]: "); break;
    case WARN:  msg.append("[WARN ]: "); break;
    case ERROR: msg.append("[ERROR]: "); break;
    default:    msg.append("[UNKN ]: "); break;
    }
}

void Logger::write(LogLevel level, const char* format, ...)
{
    if (_M_level > level) {
        return;
    }

    std::string msg;

    // 1.填充标题头
    append_with_level_title(level, msg);
    
    // 2.填充时间  
    msg.append(TimeStamp::now().to_string() + ' ');

    // 3.填充参数列表
    char buf[512] = {0};
    {
        va_list args;   // 可变参列表
        va_start(args, format);
        vsnprintf(buf, sizeof(buf), format, args);
        va_end(args);
    }
    msg.append(buf, strlen(buf));

    // 4. 输出日志
    _M_output_func(msg.data(), msg.size());
}
