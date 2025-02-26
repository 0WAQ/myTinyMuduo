#ifndef EVENTLOOPTHREADPOOL_H
#define EVENTLOOPTHREADPOOL_H

#include <functional>
#include <memory>
#include <vector>
#include <string>
#include "noncopyable.h"

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : noncopyable
{
public:

    using ThreadInitCallback = std::function<void(EventLoop*)>;

public:

    EventLoopThreadPool(EventLoop *main_loop, const std::string &name);

    /**
     * @brief
     */
    void start(const ThreadInitCallback &cb = ThreadInitCallback{});

    EventLoop* get_next_loop();

    std::vector<EventLoop*> get_all_loops();

    void set_thread_num(int num) { _M_num_threads = num; }
    bool started() const { return _M_started; }
    const std::string name() const { return _M_name; }

private:

    EventLoop *_M_main_loop;

    // EventLoop线程
    std::vector<std::unique_ptr<EventLoopThread>> _M_threads;
    
    // EventLoop线程对应的loop
    std::vector<EventLoop*> _M_sub_loops;

    std::string _M_name;

    bool _M_started;

    int _M_num_threads;

    int _M_next;

};

#endif // EVENTLOOPTHREADPOOL_H