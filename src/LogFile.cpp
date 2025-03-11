#include "LogFile.h"
#include <cstdio>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cassert>

namespace mymuduo
{

namespace __detail
{

File::File(std::string filename) :
        _M_written(0)
{
    _M_fp = ::fopen(filename.c_str(), "ae");  // 'e'表示 O_CLOEXEC
    assert(_M_fp != nullptr);
    ::setbuffer(_M_fp, _M_buf, sizeof(_M_buf));
}

File::~File() {
    ::fclose(_M_fp);
}

void File::append(const char* logline, size_t len)
{
    size_t written = 0;

    while(written != len)
    {
        size_t remain = len - written;
        size_t n = write(logline + written, remain);

        if(n != remain)
        {
            int err = ::ferror(_M_fp);
            if(err)
            {
                ::fprintf(stderr, "File::append() failed %d.\n", err);
                break;
            }
        }

        written += n;
    }

    _M_written += written;
}

void File::flush() {
    ::fflush(_M_fp);
}

size_t File::write(const char* logline, size_t len)
{
    return ::fwrite_unlocked(logline, 1, len, _M_fp);
}

} // namespace __detail

LogFile::LogFile(const std::string& basename, off_t roll_size,
    bool thread_safe, int flush_interval, int check_every) : 
        _M_basename(basename),
        _M_roll_size(roll_size),
        _M_flush_interval(flush_interval),
        _M_check_every(check_every),
        _M_count(0),
        _M_mutex(thread_safe ? new std::mutex : nullptr),
        _M_start_of_period(0),
        _M_last_roll(0),
        _M_last_flush(0)
{
    roll_file();
}

void LogFile::append(const char* logline, int len)
{
    if(_M_mutex) {
        std::lock_guard<std::mutex>(*_M_mutex);
        append_unlocked(logline, len);
    }
    else {
        append_unlocked(logline, len);
    }
}

void LogFile::flush()
{
    if(_M_mutex) {
        std::lock_guard<std::mutex>(*_M_mutex);
        _M_file->flush();
    }
    else {
        _M_file->flush();
    }
}

void LogFile::append_unlocked(const char* logline, size_t len)
{
    _M_file->append(logline, len);

    if(_M_file->written() > _M_roll_size)
    {
        roll_file();
    }
    else
    {
        _M_count++;

        if(_M_count >= _M_check_every)
        {
            _M_count = 0;

            time_t now = ::time(NULL);
            time_t thisPeriod = now / kRollPerSeconds_ * kRollPerSeconds_;
            if(thisPeriod != _M_start_of_period)
            {
                roll_file();
            }
            else if(now - _M_last_flush > _M_flush_interval)
            {
                _M_last_flush = now;
                _M_file->flush();
            }
        }
    }
}

bool LogFile::roll_file()
{
    time_t now = 0;
    std::string filename = get_logfile_name(_M_basename, &now);
    time_t start = now / kRollPerSeconds_ * kRollPerSeconds_;

    if(now > _M_last_roll)
    {
        _M_last_roll = now;
        _M_last_flush = now;
        _M_start_of_period = now;

        _M_file.reset(new __detail::File(filename));
        return true;
    }
    return false;
}

std::string LogFile::get_logfile_name(const std::string& basename, time_t *now)
{
    std::string filename;
    filename.reserve(basename.size() + 64);
    filename = basename;

    std::string hostname;
    char buf[256];
    if(::gethostname(buf, sizeof(buf)) == 0) {
        hostname = buf;
    }
    else {
        hostname = "unknownhost";
    }

    char timebuf[32];
    struct tm tm;
    *now = time(NULL);
    localtime_r(now, &tm);
    strftime(timebuf, sizeof(timebuf), ".%Y%m%d-%H%M%S.", &tm);

    filename += timebuf;
    filename += hostname;

    char pidbuf[32];
    snprintf(pidbuf, sizeof(pidbuf), ".%d", ::getpid());
    filename += pidbuf;

    filename += ".log";

    return filename;
}

} // namespace mymuduo
