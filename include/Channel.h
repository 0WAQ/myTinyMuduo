#pragma once
#include <sys/epoll.h>
#include <functional>

#include "EventLoop.h"
#include "Socket.h"

class Socket;
class EventLoop;

/**
 * 
 * 包含了一个EventLoop*的成员变量(封装了一个Epoll对象)
 * 封装了让Epoll对象监视Socket对象所需的各种变量
 */
class Channel
{
public:

    using ReadCallback = std::function<void()>;
    using WriteCallback = std::function<void()>;
    using CloseCallback = std::function<void()>;
    using ErrorCallback = std::function<void()>;

public:


    /// @brief 将fd与事件循环绑定
    /// @param loop_ptr 事件循环
    /// @param fd fd
    Channel(EventLoop* loop_ptr, int fd);


    /// @brief 事件发生后的处理函数
    void handle();


    /// @brief 四种事件
    void set_read_events();
    void set_write_events();
    void unset_read_events();
    void unset_write_events();
    void unset_all_events();



    /// @brief 从事件循环中删除Channel
    void remove();


    /// @brief 用来设置和判断该channel是否已被监视
    void set_in_epoll();
    bool in_epoll();


    /// @brief 设置边缘触发
    void set_ET();


    /// @brief 设置, 获取事件
    /// @param events 
    void set_happened_events(uint32_t events);
    uint32_t get_happened_events();
    uint32_t get_monitored_events();



    /// @brief 设置回调函数
    /// @param func 函数对象
    void set_read_callback(ReadCallback func);  // 读事件
    void set_write_callback(WriteCallback func); // 写事件
    void set_close_callback(CloseCallback func); // 关闭连接
    void set_error_callback(ErrorCallback func); // 错误事件


    /// @brief 获取channel的fd
    /// @return fd
    int get_fd();


private:
    // channel两边的fd和loop
    const int _M_fd = -1;
    EventLoop* _M_loop_ptr = nullptr;
    
    bool _M_in_epoll = false;
    uint32_t _M_monitored_events = 0;
    uint32_t _M_happened_events = 0;

    // 读事件的回调函数, 将回调Acceptor::new_connection或者new_message
    ReadCallback _M_read_callback; 

    // 写事件的回调函数, 将回调Connection::send
    WriteCallback _M_write_callback;

    // 连接关闭的回调函数, 将回调Connection::close_callback
    CloseCallback _M_close_callback; 

    // 连接出错的回调函数, 将回调Connection::error_callback
    ErrorCallback _M_error_callback; 
};