#include "mymuduo/base/CurrentThread.h"

#include <unistd.h>
#include <ctime>
#include <sys/syscall.h>

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

} // namespace CurrentThread

} // namespace mymuduo
