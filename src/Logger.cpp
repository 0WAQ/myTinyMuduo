#include "../include/Logger.hpp"

Logger::Logger() : _M_buffer(2), _M_thread_num(1), _M_pool(new ThreadPool("LOG", 1))
{

}

Logger* Logger::get_instance() {
    static Logger log;
    return &log;
}

void Logger::set_log_level(LogLevel level) {
    std::lock_guard<std::mutex> grd(_M_mutex);
    _M_level = level;
}

void Logger::append_level_title() 
{
    switch (_M_level) 
    {
    case 0:
        _M_buffer.append("[INFO] : ", 9);
        break;
    case 1:
        _M_buffer.append("[DEBUF]: ", 9);
        break;
    case 2:
        _M_buffer.append("[WRAN] : ", 9);
        break;
    case 3:
        _M_buffer.append("[ERROR]: ", 9);
        break;
    default:
        _M_buffer.append("[INFO] : ", 9);
        break;
    }
}

void Logger::write(const char* format, ...)
{   
    // 1.填充标题头
    append_level_title();
    

    // 2.填充时间
    std::string now = TimeStamp::now().to_string() + " ";
    _M_buffer.append(now.data(), now.size());


    // 3.填充参数列表
    char buf[256] = {0};
    {
        va_list args;
        va_start(args, format);
        vsnprintf(buf, sizeof(buf), format, args);
        va_end(args);
    }
    _M_buffer.append_with_sep(buf, strlen(buf));


    // 4.将任务交给LOG线程
    std::string msg;
    _M_buffer.pick_datagram(msg);
    {
        std::lock_guard<std::mutex> grd(_M_mutex);
        _M_pool->push(std::bind(&Logger::write_async, this, msg));
    }
}

void Logger::write_async(std::string msg)
{   
    {
        std::lock_guard<std::mutex> grd(_M_mutex);
        std::cout << msg << std::flush;
    }
}

Logger::~Logger()
{
    
}