#ifndef MYMUDUO_BASE_LOGFILE_H
#define MYMUDUO_BASE_LOGFILE_H

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
    off_t written() const { return _written; }

private:
    size_t write(const char* logline, size_t len);

private:
    FILE *_fp;
    char _buf[64*1024];
    off_t _written;
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

    const std::filesystem::path& filepath() const noexcept { return _filepath; }
    const std::string& basename() const noexcept { return _basename; }
    const off_t roll_size() const noexcept { return _roll_size; }
    const std::chrono::seconds flush_interval() const noexcept { return _flush_interval; }
    const int check_every() const noexcept { return _check_every; }

private:
    void append_unlocked(const char* logline, size_t len);
    static std::string get_logfile_name(const std::string& path_and_name, Timestamp& now);

private:
    const std::filesystem::path _filepath;
    const std::string _basename;
    const std::chrono::seconds _flush_interval;    // 刷新缓冲区的间隔时间
    const off_t _roll_size;                        // 文件滚动大小 (Byte)
    const size_t _check_every;                     // 文件最大行数 (Line)

    size_t _count;   // 当前文件行数

    std::unique_ptr<std::mutex> _mutex_ptr;
    Timestamp _start_of_period;  // 
    Timestamp _last_roll;        // 上次滚动文件时间
    Timestamp _last_flush;       // 上次刷新文件时间
    std::unique_ptr<__detail::File> _file;

    // 文件滚动时间
    const std::chrono::seconds kRollTime = std::chrono::days(1);
};

} // namespace mymuduo

#endif // MYMUDUO_BASE_LOGFILE_H
