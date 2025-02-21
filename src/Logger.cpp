#include "Logger.h"

Logger::Logger() : 
    _M_buffer(2), _M_fp(nullptr),
    _M_thread_num(1), _M_pool(new ThreadPool("LOG", 1))
{ }

void Logger::init(std::string path, std::string suffix)
{
    _M_dir_name = path;
    _M_suffix_name = suffix;

    _M_line_cnt = 0;

    std::string now = TimeStamp::now().to_string() + " ";
    std::string date = now.substr(0, now.find(' ', 0));


    std::string full_name{_M_dir_name + '/' + date + _M_suffix_name};

    _M_today = date;

    {
        std::unique_lock<std::mutex> grd(_M_mutex);

        if(_M_fp) {
            fclose(_M_fp);
        }

        _M_fp = fopen(full_name.c_str(), "a");
        if(_M_fp == nullptr) {
            mkdir(_M_dir_name.c_str(), 0777);
            _M_fp = fopen(full_name.c_str(), "a");
        }
        assert(_M_fp != nullptr);
    }
}

void Logger::set_level(LogLevel level) {
    std::unique_lock<std::mutex> grd(_M_mutex);
    _M_level = level;
}

void Logger::append_level_title(std::string& msg) 
{
    switch (_M_level) 
    {
    case 0:
        msg.append("[DEBUG]: ");
        break;
    case 1:
        msg.append("[DEBUG]: ");
        break;
    case 2:
        msg.append("[WRAN] : ");
        break;
    case 3:
        msg.append("[ERROR]: ");
        break;
    case 4:
        msg.append("[FATAL]: ");
        break;
    default:
        msg.append("[INFO] : ");
        break;
    }
}

void Logger::write(const char* format, ...)
{   
    // 根据当天日期创建文件
    std::string now = TimeStamp::now().to_string() + " ";   // 获取当前时间
    std::string date = now.substr(0, now.find(' ', 0)); // 根据每天的日期, 获取文件名
    if(_M_today != date || _M_line_cnt == _M_max_lines) // 当日志日期改变 或者 行数为0时, 创建新文件夹
    { 
        std::string full_name;

        // 新的一天
        if(_M_today != date) {
            full_name = _M_dir_name + '/' + date + _M_suffix_name;
            _M_today = date;
            _M_line_cnt = 0;
        }
        // 当天日志超过最大条数
        else {
            full_name = _M_dir_name + '/' + date + '_' + 
                    std::to_string((_M_line_cnt / _M_max_lines)) + _M_suffix_name;
        }

        std::unique_lock<std::mutex> grd(_M_mutex);
        fclose(_M_fp);
        _M_fp = fopen(full_name.c_str(), "a");
        assert(_M_fp != nullptr);
    }

    // 填充日志信息
    {
        std::unique_lock<std::mutex> grd(_M_mutex);

        // 待填充的信息
        std::string msg;

        // 1.填充标题头
        append_level_title(msg);
        
        // 2.填充时间  
        msg.append(now);

        // 3.填充参数列表
        char buf[512] = {0};
        {
            va_list args;   // 可变参列表
            va_start(args, format);
            vsnprintf(buf, sizeof(buf), format, args);
            va_end(args);
        }
        msg.append(buf, strlen(buf));

        // 4.将填充好的消息加到缓冲区
        _M_buffer.append(msg);

        // 5.取出消息, 将任务交给LOG线程
        _M_buffer.pick_datagram(msg);
        _M_pool->push(std::bind(&Logger::write_async, this, msg));
    }
}

void Logger::write_async(std::string msg)
{   
    {
        std::unique_lock<std::mutex> grd(_M_mutex);
        fputs(msg.c_str(), _M_fp);
        fflush(_M_fp);
    }
}

Logger::~Logger()
{
    if(_M_fp) {
        std::unique_lock<std::mutex> grd(_M_mutex);
        fclose(_M_fp);
    }
}