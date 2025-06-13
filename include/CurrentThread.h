#ifndef CURRENTTHREAD_H
#define CURRENTTHREAD_H

#include <cstdint>

namespace mymuduo
{

namespace CurrentThread
{
    extern __thread int t_cached_tid;

namespace __detail
{
    /**
     * @brief 缓存当前线程的tid
     */
    void cache_tid();

} // namespace __detail

    /**
     * @brief 获取当前线程的tid
     */
    inline int tid() {
        if(__builtin_expect(t_cached_tid == 0, 0)) {
            __detail::cache_tid();
        }
        return t_cached_tid;
    }

    /**
     * @brief 睡眠当前线程
     */
    void sleep_usec(int64_t usec);


} // namespace CurrentThread

} // namespace mymuduo

#endif // CURRENTTHREAD_H