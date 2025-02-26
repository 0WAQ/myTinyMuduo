#include "EventLoopThread.h"
#include "EventLoop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb, const std::string &name) :
                _M_loop(nullptr), _M_exiting(false),
                _M_thread(std::bind(&EventLoopThread::thread_func, this), name),
                _M_mutex(), _M_cond(), _M_init_callback(cb)
{ }

EventLoopThread::~EventLoopThread() {
    _M_exiting = true;
    if(_M_loop) {
        _M_loop->quit();
        _M_thread.join();
    }
}

EventLoop* EventLoopThread::start_loop() {

    // 启动线程
    _M_thread.start();

    EventLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(_M_mutex);

        // 等待线程获取loop, 该loop存储在线程的栈上
        while(!_M_loop) {
            _M_cond.wait(lock);
        }
        loop = _M_loop;
    }
    return loop;
}

void EventLoopThread::thread_func() {
    EventLoop loop(false);

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