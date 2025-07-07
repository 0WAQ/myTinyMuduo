#ifndef MYMUDUO_BASE_THREAD_H
#define MYMUDUO_BASE_THREAD_H

#include <thread>
#include <functional>
#include <memory>
#include <atomic>
#include <string>

#include "mymuduo/base/noncopyable.h"

namespace mymuduo {

class Thread : noncopyable {
public:
    explicit Thread(std::function<void()> func, const std::string &name = std::string{});
    explicit Thread(Thread&& other);
    Thread& operator= (Thread&& other);
    ~Thread();

    void start();
    void join();

    bool started() const noexcept { return _started.load(); }
    bool joined() const noexcept { return _joined.load(); }
    bool joinable() const noexcept { return _thread->joinable(); }
    pid_t tid() const noexcept { return _tid; }
    const std::string& name() const noexcept { return _name; }
    static const int num_created() noexcept { return _num_created; }

private:
    void set_default_name();

private:
    std::atomic<bool> _started;
    std::atomic<bool> _joined;

    pid_t _tid;
    std::unique_ptr<std::thread> _thread;

    std::function<void()> _func;

    std::string _name;

    static std::atomic<size_t> _num_created;
};

} // namespace mymuduo

#endif // MYMUDUO_BASE_THREAD_H