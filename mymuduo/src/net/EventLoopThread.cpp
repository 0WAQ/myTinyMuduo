#include "mymuduo/base/Logger.h"
#include "mymuduo/net/EventLoopThread.h"
#include "mymuduo/net/EventLoop.h"

using namespace mymuduo;
using namespace mymuduo::net;

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb, const std::string &name) :
                _loop(nullptr),
                _running(false), _exited(false),
                _thread(std::bind(&EventLoopThread::thread_func, this), name),
                _init_callback(cb)
{ }

EventLoopThread::~EventLoopThread() {
    stop_loop();
}

EventLoop* EventLoopThread::start_loop(std::chrono::nanoseconds timeout) {

    if (_running.load()) {
        return _loop;
    }

    _running.store(true);

    // 启动线程
    _thread.start();

    EventLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(_mutex);

        // 等待EventLoop线程获取loop对象, 该loop存储在线程的栈上
        if (timeout.count() > 0) {  // 超时等待
            const auto time_point = std::chrono::system_clock::now() + timeout;
            while (!_loop) {
                if (_cond.wait_until(lock, time_point) == std::cv_status::timeout)
                {
                    // 超时处理
                    if (!_loop) {
                        LOG_ERROR("EventLoopThread start_loop timeout after {} ns",
                            timeout.count());
                        return nullptr;
                    }
                    break;
                }
            }
        }
        else {  // 无限等待
            while(!_loop) {
                _cond.wait(lock);
            }
        }

        loop = _loop;
    }
    return loop;
}

void EventLoopThread::stop_loop() {
    if (!_running || _exited) {
        return;
    }

    _exited.store(true);
    _running.store(false);

    if (_loop) {
        _loop->run_in_loop([loop = _loop] {
            loop->quit();
        });
    }

    if (!_thread.joined()) {
        _thread.join();
    }
}

void EventLoopThread::thread_func() {
    
    // 将EventLoop声明为栈对象, 这样在线程退出时, EventLoop会自动析构
    EventLoop loop;

    if(_init_callback) {
        _init_callback(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(_mutex);
        _loop = &loop;
        _cond.notify_one();
    }

    // 启动事件循环
    loop.loop();

    {
        std::lock_guard<std::mutex> guard(_mutex);
        _loop = nullptr;
    }
}

