#include "base/Logger.h"
#include "net/EventLoopThread.h"
#include "net/EventLoop.h"

#include <chrono>
#include <condition_variable>

using namespace mymuduo;
using namespace mymuduo::net;

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb, const std::string &name) :
                _M_loop(nullptr),
                _M_running(false), _M_exited(false),
                _M_thread(std::bind(&EventLoopThread::thread_func, this), name),
                _M_init_callback(cb)
{ }

EventLoopThread::~EventLoopThread() {
    stop_loop();
}

EventLoop* EventLoopThread::start_loop(std::chrono::nanoseconds timeout) {

    if (_M_running.load()) {
        return _M_loop;
    }

    _M_running.store(true);

    // 启动线程
    _M_thread.start();

    EventLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(_M_mutex);

        // 等待EventLoop线程获取loop对象, 该loop存储在线程的栈上
        if (timeout.count() > 0) {  // 超时等待
            const auto time_point = std::chrono::system_clock::now() + timeout;
            while (!_M_loop) {
                if (_M_cond.wait_until(lock, time_point) == std::cv_status::timeout)
                {
                    // 超时处理
                    if (!_M_loop) {
                        LOG_ERROR("EventLoopThread start_loop timeout after %ld ns\n",
                            timeout.count());
                        return nullptr;
                    }
                    break;
                }
            }
        }
        else {  // 无限等待
            while(!_M_loop) {
                _M_cond.wait(lock);
            }
        }

        loop = _M_loop;
    }
    return loop;
}

void EventLoopThread::stop_loop() {
    if (!_M_running || _M_exited) {
        return;
    }

    _M_exited.store(true);
    _M_running.store(false);

    if (_M_loop) {
        _M_loop->run_in_loop([loop = _M_loop] {
            loop->quit();
        });
    }

    if (!_M_thread.joined()) {
        _M_thread.join();
    }
}

void EventLoopThread::thread_func() {
    
    // 将EventLoop声明为栈对象, 这样在线程退出时, EventLoop会自动析构
    EventLoop loop;

    if(_M_init_callback) {
        _M_init_callback(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(_M_mutex);
        _M_loop = &loop;
        _M_cond.notify_one();
    }

    // 启动事件循环
    loop.loop();

    {
        std::lock_guard<std::mutex> guard(_M_mutex);
        _M_loop = nullptr;
    }
}

