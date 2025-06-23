#ifndef EVENTLOOPTHREAD_H
#define EVENTLOOPTHREAD_H

#include <chrono>
#include <condition_variable>
#include <string>
#include <mutex>

#include "base/Thread.h"
#include "base/noncopyable.h"
#include "net/callbacks.h"

using namespace std::chrono_literals;

namespace mymuduo {
namespace net {

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
    EventLoopThread(const ThreadInitCallback &cb = {}, const std::string &name = {});

    ~EventLoopThread();

    /**
     * @brief 让线程启动事件循环
     * @param timeout 等待线程启动的时间, 默认为无限等待
     */
    EventLoop* start_loop(std::chrono::nanoseconds timeout = std::chrono::hours::max());

    void stop_loop();

    const bool started() const { return _M_thread.started(); }
    const bool running() const { return _M_running.load(); }
    const bool exited() const { return _M_exited.load(); }
    EventLoop* get_loop() { return _M_loop; }

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

    std::atomic<bool> _M_running;
    std::atomic<bool> _M_exited;

    std::mutex _M_mutex;

    std::condition_variable _M_cond;

    ThreadInitCallback _M_init_callback;
};

} // namespace net
} // namespace mymuduo

#endif // EVENTLOOPTHREAD_H
