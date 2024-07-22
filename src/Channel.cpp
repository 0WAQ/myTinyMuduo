#include "../include/Channel.hpp"
#include "../include/Connection.hpp"

Channel::Channel(EventLoop* loop_ptr, int fd) : 
        _M_loop_ptr(loop_ptr), _M_fd(fd) 
{ }

// 判断该fd对应的channel发生的是什么事件
void Channel::handle()
{   
    // 连接中断事件
    if(_M_happened_events & EPOLLRDHUP) 
        _M_close_callback();

    // 读事件
    else if(_M_happened_events & (EPOLLIN | EPOLLPRI)) 
        _M_read_callback();

    // 写事件
    else if(_M_happened_events & EPOLLOUT) 
        _M_write_callback();

    // 错误
    else 
        _M_error_callback(); 
    
}

// 取消对该fd的监视
void Channel::remove()
{
    unset_all_events();         // 先取消全部的事件
    _M_loop_ptr->remove_channel(this); // 从红黑树上删除fd
}

/// 获取于设置内部成员变量
int Channel::get_fd() { return _M_fd;}
bool Channel::in_epoll() {return _M_in_epoll;}
void Channel::set_in_epoll() {_M_in_epoll = true;}
void Channel::set_happened_events(uint32_t events) {_M_happened_events = events;}
uint32_t Channel::get_happened_events() {return _M_happened_events;}
uint32_t Channel::get_monitored_events() {return _M_monitored_events;}

// 设置边缘触发
void Channel::set_ET() { _M_monitored_events |= EPOLLET;}

// 设置监听事件
#define updata _M_loop_ptr->updata_channel(this)
void Channel::set_read_events() {    _M_monitored_events |= EPOLLIN;   updata;}
void Channel::set_write_events() {   _M_monitored_events |= EPOLLOUT;  updata;}
void Channel::unset_read_events() {  _M_monitored_events &= ~EPOLLIN;  updata;}
void Channel::unset_write_events() { _M_monitored_events &= ~EPOLLOUT; updata;} 
void Channel::unset_all_events() {   _M_monitored_events = 0;          updata;}

// 读,写,关闭,错误 四个设置回调函数
void Channel::set_read_callback (ReadCallback func) {_M_read_callback  = std::move(func);}
void Channel::set_write_callback(WriteCallback func) {_M_write_callback = std::move(func);}
void Channel::set_close_callback(CloseCallback func) {_M_close_callback = std::move(func);}
void Channel::set_error_callback(ErrorCallback func) {_M_error_callback = std::move(func);}
