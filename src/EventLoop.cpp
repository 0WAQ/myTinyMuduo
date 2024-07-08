#include "../include/EventLoop.hpp"

// 用于创建timerfd
int create_timerfd(time_t sec = 5, time_t nsec = 0)
{
    int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
    
    itimerspec timeout;
    memset(&timeout, 0, sizeof(itimerspec));
    timeout.it_value.tv_sec = sec;
    timeout.it_value.tv_nsec = nsec;

    timerfd_settime(tfd, 0, &timeout, 0);

    return tfd;
}

EventLoop::EventLoop(bool main_loop, time_t sec, time_t nsec) : _M_ep_ptr(new Epoll), 
        _M_efd(eventfd(0, EFD_NONBLOCK)), _M_ech(new Channel(this, _M_efd)), 
        _M_tfd(create_timerfd(sec, nsec)), _M_tch(new Channel(this, _M_tfd)),
        _M_is_main_loop(main_loop)
{
    // 监听efd的读事件
    _M_ech->set_read_events();
    // 设置读事件发生后的回调函数
    _M_ech->set_read_callback(std::bind(&EventLoop::handle_eventfd, this));

    // 监听tfd的读事件
    _M_tch->set_read_events();
    // 设置其回调函数
    _M_tch->set_read_callback(std::bind(&EventLoop::handle_timerfd, this, sec, nsec));
}

bool EventLoop::is_loop_thread() {
    return _M_tid == syscall(SYS_gettid);
}

void EventLoop::push(std::function<void()> task) 
{
    ////////////////////////////////////////////////
    {   
        std::lock_guard<std::mutex> lock(_M_mutex);
        _M_task_queue.push(task);
    }   
    ////////////////////////////////////////////////

    /**
     * 
     * 在线程池中, 用条件变量唤醒休眠的工作线程, 
     * 而在IO线程中, 其本来就在运行态, 
     * 所以用eventfd在当前epoll中注册读事件相当于唤醒
     * 
     */

    // 唤醒事件循环
    notify_one(); // 唤醒后, epoll会响应efd的读事件, 然后让其去执行handle_eventfd
}

void EventLoop::notify_one()
{
    uint64_t val = 1;
    write(_M_efd, &val, sizeof(val));
}

void EventLoop::handle_eventfd()
{
    printf("thread is %d, IO thread is waked up\n", syscall(SYS_gettid));

    uint64_t val;
    read(_M_efd, &val, sizeof(val)); // 读出来, 否则在LT中会一直触发

    // 开始执行任务
    std::function<void()> task;

    ///////////////////////////////////////////////////
    {
        std::lock_guard<std::mutex> lock(_M_mutex);
        
        // 处理任务队列中的所有任务
        while(!_M_task_queue.empty())
        {
            task = std::move(_M_task_queue.front());
            _M_task_queue.pop();
            
            task();
        }

    }
    ///////////////////////////////////////////////////

}

// 闹钟想时
void EventLoop::handle_timerfd(time_t sec, time_t nsec)
{
    // 重新计时
    itimerspec timeout;
    memset(&timeout, 0, sizeof(itimerspec));
    timeout.it_value.tv_sec = sec;
    timeout.it_value.tv_nsec = nsec;

    timerfd_settime(_M_tfd, 0, &timeout, 0);

    if(_M_is_main_loop) // 若为主线程
    {
        std::cout << "主线程的闹钟时间到了!!!\n";
    }
    else // 若为从线程
    {
        std::cout << "从线程的闹钟时间到了!!!\n";
    }
}

void EventLoop::run()
{

    // 初始化tid
    _M_tid = syscall(SYS_gettid);

    while(true)
    {
        std::vector<Channel*> channels = _M_ep_ptr->wait(10*1000);

        // 若channels为空, 则说明超时, 通知TcpServer
        if(channels.empty()) 
        {
            if(_M_epoll_wait_timeout_callback)
                _M_epoll_wait_timeout_callback(this);
        }
        else 
        {
            for(auto& ch : channels) 
                ch->handle();
        }
    }
}

void EventLoop::updata_channel(Channel* ch_ptr) {
    _M_ep_ptr->updata_channel(ch_ptr);
}

void EventLoop::remove(Channel* ch_ptr) {
    _M_ep_ptr->remove(ch_ptr);
}

void EventLoop::set_epoll_timeout_callback(std::function<void(EventLoop*)> func) {
    _M_epoll_wait_timeout_callback = func;
}

EventLoop::~EventLoop() {

}