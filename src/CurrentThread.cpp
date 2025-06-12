#include <unistd.h>
#include <cstdint>
#include <ctime>
#include <sys/syscall.h>

#include "TimeStamp.h"

namespace mymuduo
{

namespace CurrentThread
{
    __thread int t_cached_tid = 0;

namespace __detail
{
    void cache_tid() {
        if(t_cached_tid == 0) {
            t_cached_tid = static_cast<pid_t>(::syscall(SYS_gettid));
        }
    }

} // namespace __detail

    void sleep_usec(int64_t usec)
    {
        struct timespec tmspc{};
        tmspc.tv_sec = static_cast<time_t>(usec / TimeStamp::kMicroSecondsPerSecond);
        tmspc.tv_nsec = static_cast<long>(usec % TimeStamp::kMicroSecondsPerSecond * 1000);
        ::nanosleep(&tmspc, nullptr);
    }

} // namespace CurrentThread

} // namespace mymuduo