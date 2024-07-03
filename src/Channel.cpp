#include "../include/Channel.hpp"
#include "../include/Connection.hpp"

Channel::Channel(EventLoop* loop_ptr, int fd) : _M_loop_ptr(loop_ptr), _M_fd(fd)
{

}

int Channel::get_fd() {
    return _M_fd;
}

void Channel::set_ET() {
    _M_monitored_events |= EPOLLET;
}

// 除了设置为读事件, 还直接调用updata_channel去添加
void Channel::set_read_events() {
    _M_monitored_events |= EPOLLIN;
    _M_loop_ptr->updata_channel(this);
}

void Channel::set_write_events()
{
    _M_monitored_events |= EPOLLOUT;
    _M_loop_ptr->updata_channel(this);
}

void Channel::unset_read_events()
{
    _M_monitored_events &= ~EPOLLIN;
    _M_loop_ptr->updata_channel(this);    
}

void Channel::unset_write_events()
{
    _M_monitored_events &= ~EPOLLOUT;
    _M_loop_ptr->updata_channel(this);
} 

void Channel::set_in_epoll() {
    _M_in_epoll = true;
}

bool Channel::get_in_epoll() {
    return _M_in_epoll;
}

void Channel::set_happened_events(uint32_t events) {
    _M_happened_events = events;
}

uint32_t Channel::get_happened_events() {
    return _M_happened_events;
}

uint32_t Channel::get_monitored_events() {
    return _M_monitored_events;
}

void Channel::handle()
{
    // 连接中断事件
    if(_M_happened_events & EPOLLRDHUP) 
    { 
        _M_close_callback();
    }
    // 读事件
    else if(_M_happened_events & (EPOLLIN | EPOLLPRI)) 
    {
        // 利用回调函数调用处理函数
        _M_read_callback();
    }
    // 写事件
    else if(_M_happened_events & EPOLLOUT) 
    {
        
    }
    // 错误
    else 
    {
        _M_error_callback();
    }
}

void Channel::set_read_callback(std::function<void()> func) {
    _M_read_callback = func;
}

void Channel::set_close_callback(std::function<void()> func) {
    _M_close_callback = func;
}

void Channel::set_error_callback(std::function<void()> func) {
    _M_error_callback = func;
}

Channel::~Channel()
{

}