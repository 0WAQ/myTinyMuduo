#ifndef EVENTLOOPTHREAD_H
#define EVENTLOOPTHREAD_H

#include <condition_variable>
#include <functional>
#include <string>
#include <mutex>

#include "Thread.h"
#include "noncopyable.h"

class EventLoop;

/**
 * @brief EventLoop线程: one loop per thread
 */
class EventLoopThread : noncopyable
{
public:

    using ThreadInitCallback = std::function<void(EventLoop*)>;

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

    void thread_func();

    Thread _M_thread;

    EventLoop *_M_loop;

    bool _M_exiting;

    std::mutex _M_mutex;

    std::condition_variable _M_cond;

    ThreadInitCallback _M_init_callback;
};

#endif // EVENTLOOPTHREAD_H
