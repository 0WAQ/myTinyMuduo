#include "base/Thread.h"
#include "base/CurrentThread.h"
#include <condition_variable>
#include <mutex>
#include <utility>

using namespace mymuduo;

std::atomic<size_t> Thread::_M_num_created = 0ul;

Thread::Thread(std::function<void()> func, const std::string &name)
    : _M_started(false), _M_joined(false)
    , _M_tid(0), _M_name(name)
    , _M_func(std::move(func))
{
    set_default_name();
}

Thread::Thread(Thread&& other)
    : _M_started(other._M_started.load()), _M_joined(other._M_joined.load())
    , _M_tid(other._M_tid), _M_name(std::move(other._M_name))
    , _M_func(std::move(other._M_func))
{
    other._M_started.store(false);
    other._M_joined.store(false);
    other._M_tid = 0;
}

Thread& Thread::operator= (Thread&& other) {
    _M_started = other._M_started.load();
    _M_joined = other._M_joined.load();
    _M_tid = other._M_tid;
    _M_name = std::move(other._M_name);
    _M_func = std::move(other._M_func);
    return *this;
}

Thread::~Thread() {
    if(_M_started.load() && !_M_joined.load()) {
        _M_thread->detach();
    }
}

void Thread::start() {
    _M_started.store(true);

    std::condition_variable cv;
    std::mutex mtx;

    // 启动子线程
    _M_thread = std::unique_ptr<std::thread>(new std::thread([&](){
        _M_tid = CurrentThread::tid();
        cv.notify_one();
        _M_func();
    }));

    // 等待子线程获取tid
    std::unique_lock<std::mutex> lock { mtx };
    cv.wait(lock, [this] {
        return _M_tid != 0;
    });
}

void Thread::join() {

    // 防止线程未启动就 join
    if (!_M_started || !_M_thread) {
        return;
    }
    
    if (_M_joined) {
        return;
    }

    _M_joined.store(true);
    _M_started.store(false);
    _M_thread->join();
}

void Thread::set_default_name() {
    int num = ++_M_num_created;
    if(_M_name.empty()) {
        char buf[32] = {0};
        snprintf(buf, sizeof(buf), "Thread%d", num);
        _M_name = buf;
    }
}
