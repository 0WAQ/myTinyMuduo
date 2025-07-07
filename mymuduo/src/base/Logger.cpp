#include "mymuduo/base/Logger.h"
#include "mymuduo/base/AsyncLogging.h"
#include "mymuduo/base/Timestamp.h"

#include <stdarg.h> // vastart va_end

using namespace mymuduo;


void Logger::set_log_level(LogLevel level) {
    _level = level;
}

bool Logger::set_output(OutputFunc func) {
    if (_initialized.load()) {
        return false;
    }
    _initialized.store(true);
    _output_func = std::move(func);
    return true;
}

bool Logger::set_async(const std::shared_ptr<AsyncLogging>& async) {
    if (_initialized.load()) {
        return false;
    }
    _initialized.store(true);
    _output_func = std::bind(&AsyncLogging::append, async.get(),
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
    if (_level > level) {
        return;
    }

    std::string msg;

    // 1.填充标题头
    append_with_level_title(level, msg);
    
    // 2.填充时间  
    msg.append(Timestamp::now().to_string() + ' ');

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
    _output_func(msg.data(), msg.size());
}
