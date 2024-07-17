#include "../include/EventLoop.hpp"

// 用于创建timerfd
int create_timerfd(time_t sec)
{
    int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
    
    itimerspec tm;
    memset(&tm, 0, sizeof(itimerspec));
    tm.it_value.tv_sec = sec;
    tm.it_value.tv_nsec = 0;

    timerfd_settime(tfd, 0, &tm, 0);

    return tfd;
}

EventLoop::EventLoop(bool main_loop, time_t timeval, time_t timeout) : 
        _M_ep_ptr(new Epoll),
        _M_timeval(timeval), _M_timeout(timeout),
        _M_efd(eventfd(0, EFD_NONBLOCK)), _M_ech(new Channel(this, _M_efd)), 
        _M_tfd(create_timerfd(_M_timeval)), _M_tch(new Channel(this, _M_tfd)),
        _M_is_main_loop(main_loop)
{
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
    // 初始化tid
    _M_tid = syscall(SYS_gettid);

    while(!_M_stop)
    {
        _M_channels.clear();
        _M_channels = _M_ep_ptr->wait();
        
        // 若channels为空, 则说明超时, 通知TcpServer
        if(_M_channels.empty()) 
        {
            if(_M_epoll_wait_timeout_callback)
                _M_epoll_wait_timeout_callback(this);
        }
        else 
        {
            for(auto& ch :_M_channels) 
                ch->handle();
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
    // printf("thread is %d, IO thread is waked up\n", syscall(SYS_gettid));

    uint64_t val;
    read(_M_efd, &val, sizeof(val)); // 读出来, 否则在LT中会一直触发

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
            if(it->second->timer_out(_M_timeout)) 
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


bool EventLoop::is_loop_thread() { return _M_tid == syscall(SYS_gettid);}

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
    write(_M_efd, &val, sizeof(val));
}


// 转调用Epoll中对应的函数
void EventLoop::updata_channel(Channel* ch) {_M_ep_ptr->updata_channel(ch); }
void EventLoop::remove(Channel* ch) { _M_ep_ptr->remove(ch);}


// 将Connection放入map容器, 用来指示WORK线程将Connection的IO任务交给哪个IO线程
void EventLoop::insert(SpConnection conn) { _M_conns[conn->get_fd()] = conn;}


// 两个超时 设置回调函数
void EventLoop::set_epoll_timeout_callback(EpollTimeoutCallback func) {_M_epoll_wait_timeout_callback = std::move(func);}
void EventLoop::set_timer_out_callback(TimeroutCallback func) {_M_timer_out_callback = std::move(func);}

EventLoop::~EventLoop() { }