#ifndef MYMUDUO_BASE_CURRENTTHREAD_H
#define MYMUDUO_BASE_CURRENTTHREAD_H

namespace mymuduo {
namespace CurrentThread {
    extern __thread int t_cached_tid;

namespace __detail {
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

} // namespace CurrentThread
} // namespace mymuduo

#endif // MYMUDUO_BASE_CURRENTTHREAD_H