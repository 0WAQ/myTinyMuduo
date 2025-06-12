#ifndef EVENTLOOPTHREAD_H
#define EVENTLOOPTHREAD_H

#include <condition_variable>
#include <string>
#include <mutex>

#include "Thread.h"
#include "callbacks.h"
#include "noncopyable.h"

namespace mymuduo
{

class EventLoop;

/**
 * @brief EventLoop线程: one loop per thread
 */
class EventLoopThread : noncopyable
{
public:
    
    /**
     * @param cb 线程初始化时的回调函数, 线程启动前被调用
     */
    EventLoopThread(const ThreadInitCallback &cb, const std::string &name);

    ~EventLoopThread();

    /**
     * @brief 让线程启动事件循环
     */
    EventLoop* start_loop();

private:

    /**
     * @brief 线程的执行函数 
     */
    void thread_func();

private:

    // EventLoop对应的线程结构
    Thread _M_thread;

    // Thread的loop
    EventLoop *_M_loop;

    bool _M_exiting;

    std::mutex _M_mutex;

    std::condition_variable _M_cond;

    ThreadInitCallback _M_init_callback;
};

} // namespace mymuduo

#endif // EVENTLOOPTHREAD_H
