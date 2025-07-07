#include "mymuduo/base/Thread.h"
#include "mymuduo/base/CurrentThread.h"

#include <utility>
#include <condition_variable>

using namespace mymuduo;

std::atomic<size_t> Thread::_num_created = 0ul;

Thread::Thread(std::function<void()> func, const std::string &name)
    : _started(false), _joined(false)
    , _tid(0), _name(name)
    , _func(std::move(func))
{
    set_default_name();
}

Thread::Thread(Thread&& other)
    : _started(other._started.load()), _joined(other._joined.load())
    , _tid(other._tid), _name(std::move(other._name))
    , _func(std::move(other._func))
{
    other._started.store(false);
    other._joined.store(false);
    other._tid = 0;
}

Thread& Thread::operator= (Thread&& other) {
    _started = other._started.load();
    _joined = other._joined.load();
    _tid = other._tid;
    _name = std::move(other._name);
    _func = std::move(other._func);
    return *this;
}

Thread::~Thread() {
    if(_started.load() && !_joined.load()) {
        _thread->detach();
    }
}

void Thread::start() {
    _started.store(true);

    std::condition_variable cv;
    std::mutex mtx;

    // 启动子线程
    _thread = std::unique_ptr<std::thread>(new std::thread([&](){
        _tid = CurrentThread::tid();
        cv.notify_one();
        _func();
    }));

    // 等待子线程获取tid
    std::unique_lock<std::mutex> lock { mtx };
    cv.wait(lock, [this] {
        return _tid != 0;
    });
}

void Thread::join() {

    // 防止线程未启动就 join
    if (!_started || !_thread) {
        return;
    }
    
    if (_joined) {
        return;
    }

    _joined.store(true);
    _started.store(false);
    _thread->join();
}

void Thread::set_default_name() {
    int num = ++_num_created;
    if(_name.empty()) {
        char buf[32] = {0};
        snprintf(buf, sizeof(buf), "Thread%d", num);
        _name = buf;
    }
}
