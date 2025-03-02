#include "EventLoopThread.h"
#include "EventLoop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb, const std::string &name) :
                _M_loop(nullptr), _M_exiting(false),
                _M_thread(std::bind(&EventLoopThread::thread_func, this), name),
                _M_init_callback(cb)
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

        // 等待EventLoop线程获取loop对象, 该loop存储在线程的栈上
        while(!_M_loop) {
            _M_cond.wait(lock);
        }
        loop = _M_loop;
    }
    return loop;
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