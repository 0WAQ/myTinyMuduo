#ifndef THREAD_H
#define THREAD_H

#include <thread>
#include <functional>
#include <memory>
#include <atomic>
#include <string>
#include <semaphore.h>

#include "noncopyable.h"

namespace mymuduo
{

class Thread : noncopyable
{
public:

    using ThreadFunc = std::function<void()>;

public:

    explicit Thread(ThreadFunc func, const std::string &name = std::string{});

    ~Thread();

    /**
     * @brief 启动线程
     */
    void start();

    /**
     * @brief 汇入线程
     */
    void join();

    bool started() const { return _M_started; }
    bool joined() const { return _M_joined; }
    pid_t tid() const { return _M_tid; }
    const std::string& name() const { return _M_name; }
    static int num_created() { return _M_num_created; }

private:

    void set_default_name();

private:

    bool _M_started;
    bool _M_joined;

    pid_t _M_tid;
    std::shared_ptr<std::thread> _M_thread;

    ThreadFunc _M_func;     // 线程的执行函数

    std::string _M_name;

    static std::atomic<int> _M_num_created;
};

} // namespace mymuduo

#endif // THREAD_H