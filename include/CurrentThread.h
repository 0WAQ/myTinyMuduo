#ifndef CURRENTTHREAD_H
#define CURRENTTHREAD_H

namespace CurrentThread
{
    extern __thread int t_cached_tid;

    /**
     * @brief 缓存当前线程的tid
     */
    void cache_tid();
    
    /**
     * @brief 获取当前线程的tid
     */
    inline int get_tid() {
        if(__builtin_expect(t_cached_tid == 0, 0)) {
            cache_tid();
        }
        return t_cached_tid;
    }

} // namespace CurrentThread

#endif // CURRENTTHREAD_H