#include "mymuduo/base/LogFile.h"
#include "mymuduo/base/Timestamp.h"

#include <cassert>

namespace mymuduo {
namespace __detail {

File::File(std::string filename) :
        _M_written(0)
{
    _M_fp = std::fopen(filename.c_str(), "ae");  // 'e'表示 O_CLOEXEC
    assert(_M_fp != nullptr);
    ::setbuffer(_M_fp, _M_buf, sizeof(_M_buf));
}

File::~File() {
    if (_M_fp) {
        std::fclose(_M_fp);
        _M_fp = nullptr;
    }
}

void File::append(const char* logline, size_t len) {
    size_t written = 0;
    while(written != len) {
        size_t remain = len - written;
        size_t n = write(logline + written, remain);
        if(n != remain) {
            int err = std::ferror(_M_fp);
            if(err) {
                std::fprintf(stderr, "File::append() failed %d.\n", err);
                break;
            }
        }
        written += n;
    }
    _M_written += written;
}

void File::flush() {
    std::fflush(_M_fp);
}

size_t File::write(const char* logline, size_t len) {
    return ::fwrite_unlocked(logline, 1, len, _M_fp);
}

} // namespace __detail
} // namespace mymuduo

using namespace mymuduo;

LogFile::LogFile(const std::filesystem::path& filepath,
                 const std::string& basename,
                 off_t roll_size,
                 bool thread_safe,
                 std::chrono::seconds flush_interval, 
                 int check_every
                )
    : _M_filepath(filepath)
    , _M_basename(basename)
    , _M_roll_size(roll_size)
    , _M_flush_interval(flush_interval)
    , _M_check_every(check_every)
    , _M_count(0)
    , _M_mutex_ptr(thread_safe ? new std::mutex : nullptr)
    , _M_start_of_period(Timestamp())
    , _M_last_roll(Timestamp())
    , _M_last_flush(Timestamp())
{
    namespace fs = std::filesystem;
    if(!fs::exists(filepath) && !fs::create_directory(filepath)) {
        fprintf(stderr, "can't create log_path directory.");
        exit(-1);
    }
    roll_file();
}

void LogFile::append(const char* logline, int len) {
    if(_M_mutex_ptr) {
        std::lock_guard<std::mutex> guard{ *_M_mutex_ptr };
        append_unlocked(logline, len);
    }
    else {
        append_unlocked(logline, len);
    }
}

void LogFile::flush() {
    if(_M_mutex_ptr) {
        std::lock_guard<std::mutex> guard{ *_M_mutex_ptr };
        _M_file->flush();
    } else {
        _M_file->flush();
    }
}

void LogFile::append_unlocked(const char* logline, size_t len) {
    using std::chrono::seconds;
    using std::chrono::duration_cast;

    _M_file->append(logline, len);

    if(_M_file->written() > _M_roll_size) {
        roll_file();
    }
    else {
        ++_M_count;
        if(_M_count >= _M_check_every) {    // 每 check_every 次检查是否需要刷新或者滚动
            _M_count = 0;

            // 检测是否跨天
            Timestamp now = Timestamp::now();
            Timestamp this_period((now.time_since_epoch() / kRollTime) * kRollTime);
            if(this_period != _M_start_of_period) {
                roll_file();
            }
            else if(now - _M_last_flush > _M_flush_interval) {
                _M_last_flush = now;
                _M_file->flush();
            }
        }
    }
}

bool LogFile::roll_file() {
    Timestamp now = Timestamp::now();
    std::string filename = get_logfile_name(_M_filepath / _M_basename, now);
    if(now > _M_last_roll) {
        _M_last_roll = now;
        _M_last_flush = now;
        _M_start_of_period = Timestamp(now.time_since_epoch() / kRollTime * kRollTime);
        _M_file.reset(new __detail::File(filename));
        return true;
    }
    return false;
}

std::string LogFile::get_logfile_name(const std::string& path_and_name, Timestamp& now) {
    std::string filename;
    filename.reserve(path_and_name.size() + 64);
    filename = path_and_name;
    filename += '.';

    // 1. 时间戳
    filename += now.to_string();
    filename += '.';


    // 2. 主机名
    std::string hostname;
    hostname.reserve(256);
    if(::gethostname(hostname.data(), hostname.size()) != 0) {
        hostname = "unknownhost";
    }
    hostname.shrink_to_fit();

    filename += hostname;
    filename += '.';


    // 3. 进程号
    char pidbuf[32];
    snprintf(pidbuf, sizeof(pidbuf), "%d", ::getpid());

    filename += pidbuf;
    filename += ".log";

    return filename;
}
