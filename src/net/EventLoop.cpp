#include "base/CurrentThread.h"
#include "base/Logger.h"
#include "base/TimeStamp.h"
#include "net/EventLoop.h"

#include <chrono>
#include <cstdio>

using namespace mymuduo;
using namespace mymuduo::net;

using namespace std::chrono_literals;

// 防止一个线程创建多个EventLoop, __thread: 表示thread local
__thread EventLoop *t_loop_in_this_thread = nullptr;

namespace mymuduo::net {
namespace __detail {

int create_eventfd()
{
    int efd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(efd < 0) {
        LOG_ERROR("%s:%s:%d eventfd create error:%d.\n", 
            __FILE__, __FUNCTION__, __LINE__, errno);
    }
    else {
        LOG_DEBUG("create a new eventfd.\n");
    }

    return efd;
}

} // namespace __detail
} // namespace mymuduo::net

EventLoop::EventLoop() : 
        _M_tid(CurrentThread::tid()),
        _M_poller(Poller::new_default_poller(this)),
        _M_timer_queue(new TimerQueue(this)),
        _M_wakeup_fd(__detail::create_eventfd()), 
        _M_wakeup_channel(new Channel(this, _M_wakeup_fd))
{
    LOG_DEBUG("EventLoop created %p in thread %d.\n", this, _M_tid);

    /**
     * one loop per thread:
     * 检测当前线程是否已经创建了EventLoop对象
     */
    if(!t_loop_in_this_thread) {
        t_loop_in_this_thread = this;
    }
    else {
        LOG_ERROR("Another EventLoop %p exists in this thread %d\n", t_loop_in_this_thread, _M_tid);
    }

    // 监听efd的读事件, 用于异步唤醒subloop线程
    _M_wakeup_channel->set_read_events();
    // 设置读事件发生后的回调函数
    _M_wakeup_channel->set_read_callback(std::bind(&EventLoop::handle_read, this));
}

EventLoop::~EventLoop()
{
    this->quit();

    _M_wakeup_channel->unset_all_events();
    _M_wakeup_channel->remove();

    ::close(_M_wakeup_fd);
    t_loop_in_this_thread = nullptr;
}

// 运行事件循环
void EventLoop::loop(std::chrono::steady_clock::duration timeout)
{
    LOG_DEBUG("EventLoop %p start looping, thread is %d.\n", this, CurrentThread::tid());

    assert(!_M_looping);
    assert(is_loop_thread());

    _M_looping = true;

    while(!_M_quit)
    {
        _M_activeChannels.clear();
        _M_poller_return_time = _M_poller->poll(&_M_activeChannels, timeout);

        for(Channel *ch : _M_activeChannels) {
            ch->handle(_M_poller_return_time);
        }

        // 用于执行task_queue中的任务
        do_pending_functors();
    }

    LOG_INFO("EventLoop %p stop looping.\n", this);

    _M_looping = false;
}

// TODO: 支持 timeoutMs
void EventLoop::loop_once(std::chrono::steady_clock::duration timeout)
{
    LOG_DEBUG("EventLoop %p excute once, thread is %d.\n", this, CurrentThread::tid());

    assert(!_M_looping);
    assert(is_loop_thread());

    _M_looping = true;

    _M_activeChannels.clear();
    _M_poller_return_time = _M_poller->poll(&_M_activeChannels, timeout);
    
    for(Channel *ch : _M_activeChannels) {
        ch->handle(_M_poller_return_time);
    }

    // 用于执行task_queue中的任务
    do_pending_functors();
    _M_looping = false;


    LOG_INFO("EventLoop %p stop looping.\n", this);
}

void EventLoop::quit() 
{
    if (!_M_looping || _M_quit) {
        return;
    }

    _M_quit = true;
    _M_looping = false;

    // 唤醒EventLoop线程, 让其处理任务队列中剩余的任务
    if(!is_loop_thread()) {
        wakeup();
    }
}

void EventLoop::handle_read()
{
    LOG_INFO("IO thread is waked up, thread is %d.\n", syscall(SYS_gettid));

    uint64_t one;
    ssize_t len = ::read(_M_wakeup_fd, &one, sizeof(one)); // 读出来, 否则在LT中会一直触发
    if(len != sizeof(one)) {
        LOG_WARN("EventLoop::handle_eventfd() reads %d bytes instead of 8.\n", len);
    }
}

void EventLoop::do_pending_functors()
{
    // 设置局部queue, 将其与_M_task_queue交换
    std::vector<Functor> functors;
    _M_calling_pending_functors = true;

    ///////////////////////////////////////////////////
    {
        std::unique_lock<std::mutex> grd(_M_queue_mutex);
        functors.swap(_M_task_queue);
    }
    ///////////////////////////////////////////////////

    // 处理任务队列中的所有任务
    for(const Functor &func : functors) {
        func();
    }

    _M_calling_pending_functors = false;
}

void EventLoop::run_in_loop(Functor task)
{
    // 在当前loop对应的线程中执行任务
    if(is_loop_thread()) {
        task();
    }

    // 在非当前loop线程中执行任务
    else {
        queue_in_loop(std::move(task));
    }
}

void EventLoop::queue_in_loop(Functor task) 
{
    {   
        std::lock_guard<std::mutex> lock(_M_queue_mutex);
        _M_task_queue.emplace_back(task);
    }

    // 1. 若当前不是EventLoop线程, 则唤醒对应的Loop线程
    // 2. 若当前是EventLoop线程, 但是其正在执行任务队列的任务, 那么防止线程之后被阻塞, 也应该wakeup
    if(!is_loop_thread() || _M_calling_pending_functors) {
        wakeup();
    }
}

// 让该事件循环触发读事件, 从而可以执行对应Channel的回调函数
void EventLoop::wakeup()
{
    // 当前loop对应的线程就会被唤醒
    uint64_t one = 1;
    size_t len = ::write(_M_wakeup_fd, &one, sizeof(one));
    if(len != sizeof(one)) {
        LOG_ERROR("EventLoop::wakeup() writes %ld bytes instead of 8.\n", len);
    }
}

TimerId EventLoop::run_at(TimeStamp time, TimerCallback func) {
    return _M_timer_queue->add_timer(time, 0ns, std::move(func));
}

TimerId EventLoop::run_after(TimeDuration delay, TimerCallback func) {
    return run_at(TimeStamp::now() + delay, std::move(func));
}
TimerId EventLoop::run_every(TimeDuration interval, TimerCallback func) {
    return _M_timer_queue->add_timer(TimeStamp::now() + interval, interval, std::move(func));
}

void EventLoop::cancel(TimerId timerId) {
    _M_timer_queue->cancel(timerId);
}

