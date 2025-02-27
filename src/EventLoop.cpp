#include "EventLoop.h"
#include "CurrentThread.h"
#include "Logger.h"

// 防止一个线程创建多个EventLoop, __thread: 表示thread local
__thread EventLoop *t_loop_in_this_thread = nullptr;

const int kPollTimeMs = 10000;      // Poller的默认超时时间

namespace __detail
{

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

int create_timerfd(time_t sec)
{
    int tfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
    
    if(tfd < 0) {
        LOG_ERROR("%s:%s:%d timerfd create error:%d.\n", 
            __FILE__, __FUNCTION__, __LINE__, errno);
    }
    else {
        LOG_DEBUG("create a new timerfd.\n");
    }

    itimerspec tm;
    memset(&tm, 0, sizeof(itimerspec));
    tm.it_value.tv_sec = sec;
    tm.it_value.tv_nsec = 0;

    timerfd_settime(tfd, 0, &tm, 0);

    return tfd;
}

} // namespace __detail

EventLoop::EventLoop(bool main_loop, time_t timeval, time_t timeout) : 
        _M_tid(CurrentThread::tid()),
        _M_poller(Poller::new_default_poller(this)),
        
        _M_wakeup_fd(__detail::create_eventfd()), 
        _M_wakeup_channel(new Channel(this, _M_wakeup_fd))
        
        // TODO: delete?
        // _M_tfd(__detail::create_timerfd(_M_timeval)), _M_tch(new Channel(this, _M_tfd)),
        // _M_is_main_loop(main_loop),
        // _M_timeval(timeval), _M_timeout(timeout)
{
    LOG_DEBUG("EventLoop created %p in thread %d.\n", this, _M_tid);

    if(t_loop_in_this_thread) {
        LOG_ERROR("Another EventLoop %p exists in this thread %d\n", t_loop_in_this_thread, _M_tid);
    }
    else {
        t_loop_in_this_thread = this;
    }

    // 监听efd的读事件, 用于异步唤醒subloop线程
    _M_wakeup_channel->set_read_events();
    // 设置读事件发生后的回调函数
    _M_wakeup_channel->set_read_callback(std::bind(&EventLoop::handle_read, this));

    // 监听tfd的读事件, 用于清理空闲Connection
    // _M_tch->set_read_events();
    // 设置其回调函数
    // _M_tch->set_read_callback(std::bind(&EventLoop::handle_timer, this));
}

EventLoop::~EventLoop()
{
    _M_wakeup_channel->unset_all_events();
    _M_wakeup_channel->remove();
    ::close(_M_wakeup_fd);

    t_loop_in_this_thread = nullptr;
}

// 运行事件循环
void EventLoop::loop()
{
    LOG_DEBUG("EventLoop %p start looping, thread is %d.\n", this, CurrentThread::tid());

    _M_looping = true;

    while(!_M_quit)
    {
        _M_activeChannels.clear();
        _M_poller_return_time = _M_poller->poll(&_M_activeChannels, kPollTimeMs);
        
        for(Channel *ch : _M_activeChannels) {
            ch->handle(_M_poller_return_time);
        }

        // 用于执行task_queue中的任务
        do_pending_functors();
    }

    LOG_INFO("EventLoop %p stop looping.\n", this);

    _M_looping = false;
}

/**
 * 1. 在loop自己的线程中调用了stop
 * 2. 在非loop线程中调用了stop, 如WORK线程(上层的业务可能会这样做)
 */
void EventLoop::quit() 
{
    // 标志位
    _M_quit = true;

    if(!is_loop_thread()) {
        // 唤醒绑定loop的线程
        wakeup();
    }
}

// eventfd的被调函数, 执行eventfd的任务, 用于处理WORK线程的任务
void EventLoop::handle_read()
{
    LOG_INFO("IO thread is waked up, thread is %d.\n", syscall(SYS_gettid));

    uint64_t one;
    ssize_t len = ::read(_M_wakeup_fd, &one, sizeof(one)); // 读出来, 否则在LT中会一直触发
    if(len != sizeof(one)) {
        LOG_WARN("EventLoop::handle_eventfd() reads %d bytes instead of 8.\n", len);
    }
}

/* TODO: add
// timerfd的被调函数, 执行timerfd的任务, 用于清理空闲Connection
void EventLoop::handle_timer()
{
    // 重新计时, 用于每隔timeout秒, 就检测是否有空闲Connection
    itimerspec tm;
    memset(&tm, 0, sizeof(itimerspec));
    tm.it_value.tv_sec = _M_timeval;
    tm.it_value.tv_nsec = 0;

    timerfd_settime(_M_tfd, 0, &tm, 0);

    if(!_M_is_main_loop) // 若为从线程
    {
        for(auto it = _M_conns.begin(); it != _M_conns.end();) 
        {
            // 空闲Connection定义为: 当前事件距离上次发送消息的时间超过timeout秒
            if(it->second->is_expired(_M_timeout)) 
            {
                // 将TcpServer中的map容器对应的conn删除
                _M_timer_out_callback(it->second);

                ///////////////////////////////////////////////////
                std::lock_guard<std::mutex> lock(_M_map_mutex);
                it = _M_conns.erase(it);
                ///////////////////////////////////////////////////
            }
            else {
                it++;
            }
        }
    }
}
*/

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

    // 唤醒loop对应的线程
    // 或者 当前loop对应的线程正在执行回调, 为了防止线程之后被阻塞, 仍然唤醒
    if(!is_loop_thread() || _M_calling_pending_functors) { // TODO: 
        wakeup(); // 唤醒后, epoll会响应efd的读事件, 然后让其去执行handle_eventfd
    }
}

// 让该事件循环触发读事件, 从而可以执行对应Channel的回调函数
void EventLoop::wakeup()
{
    // 当前loop对应的线程就会被唤醒
    uint64_t one = 1;
    size_t len = ::write(_M_wakeup_fd, &one, sizeof(one));
    if(len != sizeof(one)) {
        LOG_ERROR("EventLoop::wakeup() writes %d bytes instead of 8.\n", len);
    }
}

void EventLoop::update_channel(Channel* ch) { _M_poller->update_channel(ch); }
void EventLoop::remove_channel(Channel* ch) { _M_poller->remove_channel(ch); }
bool EventLoop::has_channel(Channel* ch) { return _M_poller->has_channel(ch); }

// 将Connection放入map容器, 用来指示WORK线程将Connection的IO任务交给哪个IO线程
void EventLoop::insert(TcpConnectionPtr conn) { _M_conns[conn->get_fd()] = conn; }


void EventLoop::set_timer_out_callback(TimeroutCallback func) {_M_timer_out_callback = std::move(func);}
