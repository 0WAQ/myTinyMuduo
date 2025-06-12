#ifndef LOGFILE_H
#define LOGFILE_H

#include <string>
#include <memory>
#include <mutex>
#include <filesystem>
#include <time.h>
#include "noncopyable.h"

namespace mymuduo
{
    
namespace __detail
{
    
class File : noncopyable
{
public:

    explicit File(std::string filename);
    ~File();

    void append(const char* logline, size_t len);

    void flush();

    off_t written() const { return _M_written; }

private:

    size_t write(const char* logline, size_t len);

private:

    FILE *_M_fp;

    char _M_buf[64*1024];

    off_t _M_written;

};

} // namespace __detail

class LogFile : noncopyable
{
public:

    LogFile(const std::filesystem::path& basename, off_t roll_size,
            bool thread_safe = true, int flush_interval = 3, int check_every = 1024);

    ~LogFile() = default;

    void append(const char* logline, int len);

    void flush();

    bool roll_file();

private:

    void append_unlocked(const char* logline, size_t len);

    /**
     * @brief 获取日志文件的名字
     */
    static std::string get_logfile_name(const std::string& basename, time_t *now);

private:

    const std::filesystem::path _M_basename;
    const off_t _M_roll_size;       // 文件滚动大小
    const int _M_flush_interval;    // 文件刷新间隔
    const int _M_check_every;       // 文件最大行数

    int _M_count;   // 记录文件行数

    std::unique_ptr<std::mutex> _M_mutex_ptr;
    time_t _M_start_of_period;  // 
    time_t _M_last_roll;        // 上次滚动文件时间
    time_t _M_last_flush;       // 上次刷新文件时间
    std::unique_ptr<__detail::File> _M_file;

    const int kRollPerSeconds_ = 60*60*24;
};

} // namespace mymuduo

#endif // LOGFILE_H
