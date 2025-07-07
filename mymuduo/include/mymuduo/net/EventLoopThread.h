#ifndef MYMUDUO_NET_EVENTLOOPTHREAD_H
#define MYMUDUO_NET_EVENTLOOPTHREAD_H

#include <chrono>
#include <condition_variable>
#include <string>
#include <mutex>

#include "mymuduo/base/Thread.h"
#include "mymuduo/base/noncopyable.h"
#include "mymuduo/net/callbacks.h"

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

    const bool started() const { return _thread.started(); }
    const bool running() const { return _running.load(); }
    const bool exited() const { return _exited.load(); }
    EventLoop* get_loop() { return _loop; }

private:

    /**
     * @brief 线程的执行函数 
     */
    void thread_func();

private:

    // EventLoop对应的线程结构
    Thread _thread;

    // Thread的loop
    EventLoop *_loop;

    std::atomic<bool> _running;
    std::atomic<bool> _exited;

    std::mutex _mutex;

    std::condition_variable _cond;

    ThreadInitCallback _init_callback;
};

} // namespace net
} // namespace mymuduo

#endif // MYMUDUO_NET_EVENTLOOPTHREAD_H
