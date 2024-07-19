#include "../include/Logger.hpp"

Logger::Logger() : _M_buffer(2)
{

}

Logger* Logger::get_instance() {
    static Logger log;
    return &log;
}

void Logger::set_log_level(LogLevel level) {
    _M_level = level;
}

void Logger::append_level_title() {
    
    std::string title;
    switch (_M_level) {
    case 0:
        title = "[INFO] : ";
        break;
    case 1:
        title = "[DEBUG]: ";
        break;
    case 2:
        title = "[WARN] : ";
        break;
    case 3:
        title = "[ERROR]: ";
        break;
    }

    // 填充level
    _M_buffer.append(title.data(), title.size());

    // 填充时间
    std::string now = TimeStamp::now().to_string() + " ";
    _M_buffer.append(now.data(), now.size());
}

void Logger::write(const char* format, ...)
{   
    append_level_title();
    
    // 填充参数列表
    va_list args;
    va_start(args, format);
    char buf[1024] = {0};
    vsprintf(buf, format, args);
    va_end(args);

    _M_buffer.append_with_sep(buf, strlen(buf));
        
    
    //////

    std::string ans;
    _M_buffer.pick_datagram(ans);
    std::cout << ans;
}

void Logger::flush()
{
    _M_buffer.clear();
}

Logger::~Logger()
{
    
}