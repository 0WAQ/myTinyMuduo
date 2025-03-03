#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"

namespace mymuduo
{

EventLoopThreadPool::EventLoopThreadPool(EventLoop *main_loop, const std::string &name) :
            _M_main_loop(main_loop), _M_name(name), _M_started(false), _M_num_threads(0), _M_next(0)
{ }

void EventLoopThreadPool::start(const ThreadInitCallback &cb) {
    _M_started = true;

    for(int i = 0; i < _M_num_threads; i++) {
        char buf[_M_name.size() + 32];
        snprintf(buf, sizeof(buf), "%s%d", _M_name.c_str(), i);

        EventLoopThread *thread = new EventLoopThread(cb, _M_name);
        _M_threads.emplace_back(std::unique_ptr<EventLoopThread>(thread));
        _M_sub_loops.emplace_back(thread->start_loop());
    }

    if(_M_num_threads == 0 && cb) {
        cb(_M_main_loop);
    }
}

EventLoop* EventLoopThreadPool::get_next_loop() {
    EventLoop *loop = _M_main_loop;
    if(!_M_sub_loops.empty()) {
        loop = _M_sub_loops[_M_next];
        if(++_M_next > _M_sub_loops.size()) {
            _M_next = 0;
        }
    }
    return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::get_all_loops() {
    if(_M_sub_loops.empty()) {
        return std::vector<EventLoop*>{1, _M_main_loop};
    }
    return _M_sub_loops;
}

} // namespace mymuduo