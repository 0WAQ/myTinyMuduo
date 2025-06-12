#include "Logger.h"
#include "TimeStamp.h"
#include <filesystem>

namespace mymuduo
{

Logger::Logger(const std::string& path, const std::string& basename, int roll_size = 500*1000) :
        _M_async_logging(path + basename, roll_size, 1),
        _M_dir_name(path)
{
    namespace fs = std::filesystem;
    fs::directory_entry entry{_M_dir_name};
    
    if(!entry.exists() && !fs::create_directory(entry)) {
        fprintf(stderr, "can't create log_path directory.");
        exit(-1);
    }
}

void Logger::init(LogLevel level)
{
    _M_level = level;

    _M_async_logging.start();
}

void Logger::set_log_level(LogLevel level) {
    _M_level = level;
}

Logger::LogLevel Logger::log_level() {
    return _M_level;
}

void Logger::append_level_title(LogLevel level, std::string& msg) 
{
    switch (level)
    {
    case 0:
        msg.append("[DEBUG]: ");
        break;
    case 1:
        msg.append("[INFO]: ");
        break;
    case 2:
        msg.append("[WARN] : ");
        break;
    case 3:
        msg.append("[ERROR]: ");
        break;
    }
}

void Logger::write(LogLevel level, const char* format, ...)
{   
    // 待填充的信息
    std::string msg;

    // 1.填充标题头
    append_level_title(level, msg);
    
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

    // 4. 前端线程写入缓冲区
    _M_async_logging.append(msg.data(), msg.size());    
}


} // namespace mymuduo