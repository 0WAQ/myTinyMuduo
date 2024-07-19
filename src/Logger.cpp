#include "../include/Logger.hpp"

Logger::Logger() : _M_buffer(2), _M_thread_num(1), _M_pool(new ThreadPool("LOG", 1))
{

}

Logger* Logger::get_instance() {
    static Logger log; // 懒汉模式, 在第一次调用时才创建对象
    return &log;
}

void Logger::set_level(LogLevel level) {
    std::unique_lock<std::mutex> grd(_M_mutex);
    _M_level = level;
}

LogLevel Logger::get_level() {
    std::unique_lock<std::mutex> grd(_M_mutex);
    return _M_level;
}

void Logger::append_level_title(LogLevel level) 
{
    switch (level) 
    {
    case 0:
        _M_buffer.append("[INFO] : ", 9);
        break;
    case 1:
        _M_buffer.append("[DEBUG]: ", 9);
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

void Logger::write(LogLevel level, const char* format, ...)
{   
    {
        std::unique_lock<std::mutex> grd(_M_mutex);

        va_list args;
        std::string msg;
        std::string now = TimeStamp::now().to_string() + " ";

        // 1.填充标题头
        append_level_title(level);
        

        // 2.填充时间  
        _M_buffer.append(now.data(), now.size());


        // 3.填充参数列表
        char buf[256] = {0};
        {    
            va_start(args, format);
            vsnprintf(buf, sizeof(buf), format, args);
            va_end(args);
        }
        _M_buffer.append_with_sep(buf, strlen(buf));


        // 4.将任务交给LOG线程
        _M_buffer.pick_datagram(msg);
        _M_pool->push(std::bind(&Logger::write_async, this, msg));
    }
}

void Logger::write_async(std::string msg)
{   
    {
        std::unique_lock<std::mutex> grd(_M_mutex);
        std::cout << msg << std::flush;
    }
}

Logger::~Logger()
{
    
}