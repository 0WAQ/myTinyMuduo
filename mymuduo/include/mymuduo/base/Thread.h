#ifndef THREAD_H
#define THREAD_H

#include <cstddef>
#include <thread>
#include <functional>
#include <memory>
#include <atomic>
#include <string>
#include <semaphore.h>

#include "mymuduo/base/noncopyable.h"

namespace mymuduo {

class Thread : noncopyable
{
public:

    explicit Thread(std::function<void()> func, const std::string &name = std::string{});

    explicit Thread(Thread&& other);
    Thread& operator= (Thread&& other);

    ~Thread();


    void start();
    void join();

    bool started() const noexcept { return _M_started.load(); }
    bool joined() const noexcept { return _M_joined.load(); }
    bool joinable() const noexcept { return _M_thread->joinable(); }
    pid_t tid() const noexcept { return _M_tid; }
    const std::string& name() const noexcept { return _M_name; }
    static const int num_created() noexcept { return _M_num_created; }

private:
    void set_default_name();

private:
    std::atomic<bool> _M_started;
    std::atomic<bool> _M_joined;

    pid_t _M_tid;
    std::unique_ptr<std::thread> _M_thread;

    std::function<void()> _M_func;

    std::string _M_name;

    static std::atomic<size_t> _M_num_created;
};

} // namespace mymuduo

#endif // THREAD_H