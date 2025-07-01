#ifndef LOGFILE_H
#define LOGFILE_H

#include <chrono>
#include <string>
#include <memory>
#include <mutex>
#include <filesystem>

#include "mymuduo/base/Timestamp.h"
#include "mymuduo/base/noncopyable.h"

namespace mymuduo {
namespace __detail {
    
class File : noncopyable {
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

class LogFile : noncopyable {
public:
    LogFile(const std::filesystem::path& filepath,
            const std::string& basename,
            off_t roll_size,
            bool thread_safe,
            std::chrono::seconds flush_interval,
            int check_every = 1024);

    ~LogFile() = default;

    void append(const char* logline, int len);

    void flush();

    bool roll_file();

    const std::filesystem::path& filepath() const noexcept { return _M_filepath; }
    const std::string& basename() const noexcept { return _M_basename; }
    const off_t roll_size() const noexcept { return _M_roll_size; }
    const std::chrono::seconds flush_interval() const noexcept { return _M_flush_interval; }
    const int check_every() const noexcept { return _M_check_every; }

private:
    void append_unlocked(const char* logline, size_t len);

    static std::string get_logfile_name(const std::string& path_name, time_t *now);

private:
    const std::filesystem::path _M_filepath;
    const std::string _M_basename;
    const off_t _M_roll_size;       // 文件滚动大小
    const std::chrono::seconds _M_flush_interval;    // 刷新缓冲区的间隔时间
    const int _M_check_every;       // 文件最大行数

    int _M_count;   // 记录文件行数

    std::unique_ptr<std::mutex> _M_mutex_ptr;
    Timestamp _M_start_of_period;  // 
    Timestamp _M_last_roll;        // 上次滚动文件时间
    Timestamp _M_last_flush;       // 上次刷新文件时间
    std::unique_ptr<__detail::File> _M_file;

    const std::chrono::seconds kRollPerSeconds = std::chrono::seconds{60 * 60 * 24};
};

} // namespace mymuduo

#endif // LOGFILE_H
