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

    void set_thread_num(int num) { _M_num_threads = num; }
    int num_threads() { return _M_num_threads; }
    bool started() const { return _M_started; }
    const std::string name() const { return _M_name; }

private:

    // 该类并不拥有main_loop, 是由上层传递而来
    EventLoop *_M_main_loop;

    // 从EventLoop线程与其对应的EventLoop对象
    std::vector<std::unique_ptr<EventLoopThread>> _M_threads;
    std::vector<EventLoop*> _M_sub_loops;

    std::string _M_name;

    std::atomic<bool> _M_started;
    std::atomic<bool> _M_exited;

    int _M_num_threads;

    std::atomic<int> _M_next;    // 下一个连接所属的EventLoop的索引

};

} // namespace net
} // namespace mymuduo

#endif // MYMUDUO_NET_EVENTLOOPTHREADPOOL_H