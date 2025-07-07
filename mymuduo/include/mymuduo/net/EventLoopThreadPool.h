#ifndef MYMUDUO_NET_EVENTLOOPTHREADPOOL_H
#define MYMUDUO_NET_EVENTLOOPTHREADPOOL_H

#include <atomic>
#include <memory>
#include <vector>
#include <string>

#include "mymuduo/base/noncopyable.h"
#include "mymuduo/net/callbacks.h"

namespace mymuduo {
namespace net {

class EventLoop;
class EventLoopThread;

/**
 * @brief EventLoop线程池, 每个服务器都要有该对象
 */
class EventLoopThreadPool : noncopyable
{
public:

    EventLoopThreadPool(EventLoop *main_loop, const std::string &name);

    ~EventLoopThreadPool();

    /**
     * @brief 启动从EventLoop线程以及对应的事件循环
     */
    void start(const ThreadInitCallback &cb = ThreadInitCallback{});
    void stop();

    EventLoop* get_next_loop(); // 非线程安全!

    std::vector<EventLoop*> get_all_loops();

    void set_thread_num(int num) { _num_threads = num; }
    int num_threads() { return _num_threads; }
    bool started() const { return _started; }
    const std::string name() const { return _name; }

private:

    // 该类并不拥有main_loop, 是由上层传递而来
    EventLoop *_main_loop;

    // 从EventLoop线程与其对应的EventLoop对象
    std::vector<std::unique_ptr<EventLoopThread>> _threads;
    std::vector<EventLoop*> _sub_loops;

    std::string _name;

    std::atomic<bool> _started;
    std::atomic<bool> _exited;

    int _num_threads;

    std::atomic<int> _next;    // 下一个连接所属的EventLoop的索引

};

} // namespace net
} // namespace mymuduo

#endif // MYMUDUO_NET_EVENTLOOPTHREADPOOL_H