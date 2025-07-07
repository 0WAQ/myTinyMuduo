#include "mymuduo/base/LogFile.h"
#include "mymuduo/base/Timestamp.h"

#include <cassert>

namespace mymuduo {
namespace __detail {

File::File(std::string filename) :
        _written(0)
{
    _fp = std::fopen(filename.c_str(), "ae");  // 'e'表示 O_CLOEXEC
    assert(_fp != nullptr);
    ::setbuffer(_fp, _buf, sizeof(_buf));
}

File::~File() {
    if (_fp) {
        std::fclose(_fp);
        _fp = nullptr;
    }
}

void File::append(const char* logline, size_t len) {
    size_t written = 0;
    while(written != len) {
        size_t remain = len - written;
        size_t n = write(logline + written, remain);
        if(n != remain) {
            int err = std::ferror(_fp);
            if(err) {
                std::fprintf(stderr, "File::append() failed %d.\n", err);
                break;
            }
        }
        written += n;
    }
    _written += written;
}

void File::flush() {
    std::fflush(_fp);
}

size_t File::write(const char* logline, size_t len) {
    return ::fwrite_unlocked(logline, 1, len, _fp);
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
    : _filepath(filepath)
    , _basename(basename)
    , _roll_size(roll_size)
    , _flush_interval(flush_interval)
    , _check_every(check_every)
    , _count(0)
    , _mutex_ptr(thread_safe ? new std::mutex : nullptr)
    , _start_of_period(Timestamp())
    , _last_roll(Timestamp())
    , _last_flush(Timestamp())
{
    namespace fs = std::filesystem;
    if(!fs::exists(filepath) && !fs::create_directory(filepath)) {
        fprintf(stderr, "can't create log_path directory.");
        exit(-1);
    }
    roll_file();
}

void LogFile::append(const char* logline, int len) {
    if(_mutex_ptr) {
        std::lock_guard<std::mutex> guard{ *_mutex_ptr };
        append_unlocked(logline, len);
    }
    else {
        append_unlocked(logline, len);
    }
}

void LogFile::flush() {
    if(_mutex_ptr) {
        std::lock_guard<std::mutex> guard{ *_mutex_ptr };
        _file->flush();
    } else {
        _file->flush();
    }
}

void LogFile::append_unlocked(const char* logline, size_t len) {
    using std::chrono::seconds;
    using std::chrono::duration_cast;

    _file->append(logline, len);

    if(_file->written() > _roll_size) {
        roll_file();
    }
    else {
        ++_count;
        if(_count >= _check_every) {    // 每 check_every 次检查是否需要刷新或者滚动
            _count = 0;

            // 检测是否跨天
            Timestamp now = Timestamp::now();
            Timestamp this_period((now.time_since_epoch() / kRollTime) * kRollTime);
            if(this_period != _start_of_period) {
                roll_file();
            }
            else if(now - _last_flush > _flush_interval) {
                _last_flush = now;
                _file->flush();
            }
        }
    }
}

bool LogFile::roll_file() {
    Timestamp now = Timestamp::now();
    std::string filename = get_logfile_name(_filepath / _basename, now);
    if(now > _last_roll) {
        _last_roll = now;
        _last_flush = now;
        _start_of_period = Timestamp(now.time_since_epoch() / kRollTime * kRollTime);
        _file.reset(new __detail::File(filename));
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
