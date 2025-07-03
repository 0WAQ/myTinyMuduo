#include "mymuduo/base/Logger.h"
#include "mymuduo/net/Channel.h"
#include "mymuduo/net/EventLoop.h"
#include "mymuduo/net/TcpConnection.h"

using namespace mymuduo;
using namespace mymuduo::net;

Channel::Channel(EventLoop* loop_ptr, int fd) : 
        _M_loop_ptr(loop_ptr),
        _M_fd(fd),
        _M_monitored_events(0),
        _M_happened_events(0),
        _M_status(kNew),
        _M_tied(false),
        _M_in_epoll(false)
{ }

// 判断该fd对应的channel发生的是什么事件
void Channel::handle(Timestamp receiveTime)
{
    // 该分支是TcpConnection对象回调会进入的分支(TcpConnecton对象在establish时调用了tie)
    std::shared_ptr<void> guard;
    if(_M_tied) {
        // MARK: 获取对象的强引用, 确保TcpConnection对象在channel回调期间存活
        guard = _M_tie.lock();
        if(guard) {
            handle_event_with_guard(receiveTime);
        }
    }
    else {
        // MARK: 该分支是Acceptor, TimerQueue, wakeup_channel回调会进入的分支(没有调用tie)
        handle_event_with_guard(receiveTime);
    }
}

void Channel::handle_event_with_guard(Timestamp receiveTime)
{
    // 连接中断事件
    if(_M_happened_events & EPOLLRDHUP) // 检测半关闭事件
    {
        LOG_DEBUG("channel handle happened close events\n");

        if(_M_close_callback) {
            _M_close_callback();
        }
    }

    // 读事件
    if(_M_happened_events & _M_read_events)
    {
        LOG_DEBUG("channel handle happened read events\n");

        if(_M_read_callback) {
            _M_read_callback(receiveTime);
        }
    }

    // 写事件
    if(_M_happened_events & _M_write_events)
    {
        LOG_DEBUG("channel handle happened write events\n");

        if(_M_write_callback) {
            _M_write_callback();
        }
    }

    // 错误
    if(_M_happened_events & EPOLLERR)
    {
        LOG_DEBUG("channel handle happened error events\n");

        if(_M_error_callback) {
            _M_error_callback(); 
        }
    }
}

void Channel::remove() {
    _M_in_epoll = false;
    _M_loop_ptr->remove_channel(this);
}

void Channel::update() {
    _M_in_epoll = true;
    _M_loop_ptr->update_channel(this);
}


/// 获取于设置内部成员变量
int Channel::fd() { return _M_fd; }
void Channel::set_happened_events(uint32_t events) { _M_happened_events = events; }
uint32_t Channel::get_happened_events() { return _M_happened_events; }
uint32_t Channel::get_monitored_events() { return _M_monitored_events; }

void Channel::tie(const std::shared_ptr<void>& obj) {
    _M_tie = obj;
    _M_tied = true;
}

// 设置边缘触发
void Channel::set_ET()   { _M_monitored_events |= EPOLLET; }
void Channel::unset_ET() { _M_monitored_events &= ~EPOLLET; }

// 设置监听事件
void Channel::set_read_events() {    _M_monitored_events |= _M_read_events;   update(); }
void Channel::set_write_events() {   _M_monitored_events |= _M_write_events;  update(); }
void Channel::unset_read_events() {  _M_monitored_events &= ~_M_read_events;  update(); }
void Channel::unset_write_events() { _M_monitored_events &= ~_M_write_events; update(); } 
void Channel::unset_all_events() {   _M_monitored_events = _M_none_events;    update(); }

// 读,写,关闭,错误 四个设置回调函数
void Channel::set_read_callback (ReadCallback cb)  { _M_read_callback  = std::move(cb); }
void Channel::set_write_callback(EventCallback cb) { _M_write_callback = std::move(cb); }
void Channel::set_close_callback(EventCallback cb) { _M_close_callback = std::move(cb); }
void Channel::set_error_callback(EventCallback cb) { _M_error_callback = std::move(cb); }

