#include "mymuduo/base/Logger.h"
#include "mymuduo/base/AsyncLogging.h"
#include "mymuduo/base/Timestamp.h"

using namespace mymuduo;

const char* LogLevelName[Logger::NUM_LOG_LEVELS] = 
{
    "[DEBUG]: ",
    "[INFO ]: ",
    "[WARN ]: ",
    "[ERROR]: ",
    "[UNKN ]: "
};


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

    if (!async) {
        _initialized.store(false);
        _output_func = [](const char* data, size_t len) { };
        return false;
    }

    _initialized.store(true);
    _output_func = std::bind(&AsyncLogging::append, async.get(),
                        std::placeholders::_1, std::placeholders::_2);
    async->start();
    return true;
}

void Logger::write(LogLevel level, std::string&& fmt)
{
    if (_level > level) {
        return;
    }
    std::string msg;

    // 1.填充标题头
    msg.append(LogLevelName[level]);
    
    // 2.填充时间  
    msg.append(Timestamp::now().to_string() + ' ');

    // 3.填充参数列表
    msg.append(fmt);
    msg.append("\n");

    // 4. 输出日志
    _output_func(msg.data(), msg.size());
}
