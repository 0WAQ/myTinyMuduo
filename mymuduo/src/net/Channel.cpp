#include "mymuduo/base/Logger.h"
#include "mymuduo/net/Channel.h"
#include "mymuduo/net/EventLoop.h"
#include "mymuduo/net/TcpConnection.h"

using namespace mymuduo;
using namespace mymuduo::net;

Channel::Channel(EventLoop* loop_ptr, int fd) : 
        _loop_ptr(loop_ptr),
        _fd(fd),
        _monitored_events(0),
        _happened_events(0),
        _status(kNew),
        _tied(false),
        _in_epoll(false)
{ }

// 判断该fd对应的channel发生的是什么事件
void Channel::handle(Timestamp receiveTime)
{
    // 该分支是TcpConnection对象回调会进入的分支(TcpConnecton对象在establish时调用了tie)
    std::shared_ptr<void> guard;
    if(_tied) {
        // MARK: 获取对象的强引用, 确保TcpConnection对象在channel回调期间存活
        guard = _tie.lock();
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
    if(_happened_events & EPOLLRDHUP) // 检测半关闭事件
    {
        LOG_DEBUG("channel handle happened close events");

        if(_close_callback) {
            _close_callback();
        }
    }

    // 读事件
    if(_happened_events & _read_events)
    {
        LOG_DEBUG("channel handle happened read events");

        if(_read_callback) {
            _read_callback(receiveTime);
        }
    }

    // 写事件
    if(_happened_events & _write_events)
    {
        LOG_DEBUG("channel handle happened write events");

        if(_write_callback) {
            _write_callback();
        }
    }

    // 错误
    if(_happened_events & EPOLLERR)
    {
        LOG_DEBUG("channel handle happened error events");

        if(_error_callback) {
            _error_callback(); 
        }
    }
}

void Channel::remove() {
    _in_epoll = false;
    _loop_ptr->remove_channel(this);
}

void Channel::update() {
    _in_epoll = true;
    _loop_ptr->update_channel(this);
}


/// 获取于设置内部成员变量
int Channel::fd() { return _fd; }
void Channel::set_happened_events(uint32_t events) { _happened_events = events; }
uint32_t Channel::get_happened_events() { return _happened_events; }
uint32_t Channel::get_monitored_events() { return _monitored_events; }

void Channel::tie(const std::shared_ptr<void>& obj) {
    _tie = obj;
    _tied = true;
}

// 设置边缘触发
void Channel::set_ET()   { _monitored_events |= EPOLLET; }
void Channel::unset_ET() { _monitored_events &= ~EPOLLET; }

// 设置监听事件
void Channel::set_read_events() {    _monitored_events |= _read_events;   update(); }
void Channel::set_write_events() {   _monitored_events |= _write_events;  update(); }
void Channel::unset_read_events() {  _monitored_events &= ~_read_events;  update(); }
void Channel::unset_write_events() { _monitored_events &= ~_write_events; update(); } 
void Channel::unset_all_events() {   _monitored_events = _none_events;    update(); }

// 读,写,关闭,错误 四个设置回调函数
void Channel::set_read_callback (ReadCallback cb)  { _read_callback  = std::move(cb); }
void Channel::set_write_callback(EventCallback cb) { _write_callback = std::move(cb); }
void Channel::set_close_callback(EventCallback cb) { _close_callback = std::move(cb); }
void Channel::set_error_callback(EventCallback cb) { _error_callback = std::move(cb); }

