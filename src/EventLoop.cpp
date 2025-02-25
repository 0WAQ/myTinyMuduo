#include "EventLoop.h"
#include "CurrentThread.h"

// 用于创建timerfd
int create_timerfd(time_t sec)
{
    int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
    
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

int create_eventfd()
{
    int efd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(efd < 0) {
        LOG_ERROR("%s:%s:%d eventfd create error:%d.\n", 
            __FILE__, __FUNCTION__, __LINE__, errno);
    }
    else {
        LOG_DEBUG("create a new eventfd.\n");
    }

    return efd;
}

EventLoop::EventLoop(bool main_loop, time_t timeval, time_t timeout) : 
        _M_poller((EPollPoller*)Poller::new_default_poller(this)),
        _M_timeval(timeval), _M_timeout(timeout),
        _M_efd(create_eventfd()), _M_ech(new Channel(this, _M_efd)), 
        _M_tfd(create_timerfd(_M_timeval)), _M_tch(new Channel(this, _M_tfd)),
        _M_is_main_loop(main_loop)
{
    LOG_DEBUG("EventLoop created, thread is %d.\n", syscall(SYS_gettid));

    // 监听efd的读事件, 用于异步唤醒事件循环线程
    _M_ech->set_read_events();
    // 设置读事件发生后的回调函数
    _M_ech->set_read_callback(std::bind(&EventLoop::handle_eventfd, this));

    // 监听tfd的读事件, 用于清理空闲Connection
    _M_tch->set_read_events();
    // 设置其回调函数
    _M_tch->set_read_callback(std::bind(&EventLoop::handle_timerfd, this));
}

// 运行事件循环
void EventLoop::loop()
{
    LOG_DEBUG("EventLoop start looping, thread is %d.\n", syscall(SYS_gettid));

    // 初始化tid
    _M_tid = CurrentThread::get_tid(); // syscall(SYS_gettid);

    while(!_M_stop)
    {
        _M_channels.clear();
        TimeStamp now = _M_poller->poll(&_M_channels);
        
        // 若channels为空, 则说明超时, 通知TcpServer
        if(_M_channels.empty())
        {
            if(_M_epoll_wait_timeout_callback)
                _M_epoll_wait_timeout_callback(this);
        }
        else 
        {
            for(auto& ch :_M_channels) {
                ch->handle(TimeStamp::now());
            }
        }
    }
}

// 停止事件循环
void EventLoop::stop() 
{
    // 标志位
    _M_stop = true;

    // 唤醒epoll_wait
    wakeup();
}

// eventfd的被调函数, 执行eventfd的任务, 用于处理WORK线程的任务
void EventLoop::handle_eventfd()
{
    LOG_INFO("IO thread is waked up, thread is %d.\n", syscall(SYS_gettid));

    uint64_t val;
    ssize_t len = read(_M_efd, &val, sizeof(val)); // 读出来, 否则在LT中会一直触发

    // 开始执行任务
    std::function<void()> task;

    ///////////////////////////////////////////////////
    {
        std::unique_lock<std::mutex> grd(_M_mutex);
        
        // 处理任务队列中的所有任务
        while(!_M_task_queue.empty())
        {
            // 队列是共享的
            task = std::move(_M_task_queue.front());
            _M_task_queue.pop();
            
            grd.unlock();
            task();
            grd.lock();
        }

    }
    ///////////////////////////////////////////////////
}

// timerfd的被调函数, 执行timerfd的任务, 用于清理空闲Connection
void EventLoop::handle_timerfd()
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
                std::lock_guard<std::mutex> lock(_M_mmutex);
                it = _M_conns.erase(it);
                ///////////////////////////////////////////////////
            }
            else {
                it++;
            }
        }
    }
}


bool EventLoop::is_loop_thread() { return _M_tid == CurrentThread::get_tid(); }

void EventLoop::push(WorkThreadCallback task) 
{
    {   
        std::lock_guard<std::mutex> lock(_M_mutex);
        _M_task_queue.push(task);
    }   

    // 唤醒事件循环
    wakeup(); // 唤醒后, epoll会响应efd的读事件, 然后让其去执行handle_eventfd
}

// 让该事件循环触发读事件, 从而可以执行对应Channel的回调函数
void EventLoop::wakeup()
{
    uint64_t val = 1;
    size_t len = write(_M_efd, &val, sizeof(val));
    if(len != sizeof(val)) {
        LOG_ERROR("EventLoop::wakeup writes %d, instead of 8.\n", len);
    }
}


// 转调用Epoll中对应的函数
void EventLoop::update_channel(Channel* ch) { _M_poller->update_channel(ch); }
void EventLoop::remove_channel(Channel* ch) { _M_poller->remove_channel(ch); }


// 将Connection放入map容器, 用来指示WORK线程将Connection的IO任务交给哪个IO线程
void EventLoop::insert(SpConnection conn) { _M_conns[conn->get_fd()] = conn;}


// 两个超时 设置回调函数
void EventLoop::set_epoll_timeout_callback(EpollTimeoutCallback func) {_M_epoll_wait_timeout_callback = std::move(func);}
void EventLoop::set_timer_out_callback(TimeroutCallback func) {_M_timer_out_callback = std::move(func);}
