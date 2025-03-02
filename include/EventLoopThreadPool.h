#ifndef EVENTLOOPTHREADPOOL_H
#define EVENTLOOPTHREADPOOL_H

#include <functional>
#include <memory>
#include <vector>
#include <string>
#include "noncopyable.h"

class EventLoop;
class EventLoopThread;

/**
 * @brief EventLoop线程池, 每个服务器都要有该对象
 */
class EventLoopThreadPool : noncopyable
{
public:

    using ThreadInitCallback = std::function<void(EventLoop*)>;

public:

    EventLoopThreadPool(EventLoop *main_loop, const std::string &name);

    /**
     * @brief 启动从EventLoop线程以及对应的事件循环
     */
    void start(const ThreadInitCallback &cb = ThreadInitCallback{});

    EventLoop* get_next_loop();

    std::vector<EventLoop*> get_all_loops();

    void set_thread_num(int num) { _M_num_threads = num; }
    bool started() const { return _M_started; }
    const std::string name() const { return _M_name; }

private:

    // 该类并不拥有main_loop, 是由上层传递而来
    EventLoop *_M_main_loop;

    // 从EventLoop线程与其对应的EventLoop对象
    std::vector<std::unique_ptr<EventLoopThread>> _M_threads;
    std::vector<EventLoop*> _M_sub_loops;

    std::string _M_name;

    bool _M_started;

    int _M_num_threads;

    int _M_next;

};

#endif // EVENTLOOPTHREADPOOL_H