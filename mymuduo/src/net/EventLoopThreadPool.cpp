#include "mymuduo/net/EventLoop.h"
#include "mymuduo/net/EventLoopThread.h"
#include "mymuduo/net/EventLoopThreadPool.h"

using namespace mymuduo;
using namespace mymuduo::net;

EventLoopThreadPool::EventLoopThreadPool(EventLoop *main_loop, const std::string &name) :
            _main_loop(main_loop), _name(name), _num_threads(0), _next(0),
            _started(false), _exited(false)
{ }

EventLoopThreadPool::~EventLoopThreadPool() {
    stop();
}

void EventLoopThreadPool::start(const ThreadInitCallback &cb) {
    _started = true;

    for(int i = 0; i < _num_threads; i++) {
        char buf[_name.size() + 32];
        snprintf(buf, sizeof(buf), "%s%d", _name.c_str(), i);

        EventLoopThread *thread = new EventLoopThread(cb, _name);
        _threads.emplace_back(std::unique_ptr<EventLoopThread>(thread));
        _sub_loops.emplace_back(thread->start_loop());
    }

    if(_num_threads == 0 && cb) {
        cb(_main_loop);
    }
}

void EventLoopThreadPool::stop() {
    if (!_started.load() || _exited.load()) {
        return;
    }

    _exited.store(true);
    _started.store(false);

    for (int i = 0; i < _num_threads; ++i) {
        // 会自动调用析构
        std::unique_ptr<EventLoopThread> thread = std::move(_threads[i]);
    }
}

EventLoop* EventLoopThreadPool::get_next_loop() {
    EventLoop *loop = _main_loop;
    if(!_sub_loops.empty()) {
        loop = _sub_loops[_next.load()];
        if(++_next >= _sub_loops.size()) {
            _next.store(0);
        }
    }
    return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::get_all_loops() {
    if(_sub_loops.empty()) {
        return std::vector<EventLoop*> { _main_loop };
    }
    return _sub_loops;
}

