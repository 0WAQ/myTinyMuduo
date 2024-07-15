#pragma once

#include <functional>
#include <unistd.h>
#include <sys/syscall.h>
#include <memory>
#include <atomic>

#include "Socket.hpp"
#include "EventLoop.hpp"
#include "Channel.hpp"
#include "Buffer.hpp"
#include "TimeStamp.hpp"

class EventLoop;
class Channel;
class Connection;
using Connection_ptr = std::shared_ptr<Connection>;


/**
 *  Channel之上的封装类, 专门用于创建客户端的Socket
 *  继承自这个类, 用shared_from_this代替this指针
 */
class Connection : public std::enable_shared_from_this<Connection>
{
public:


    /// @brief 将sock绑定到事件循环
    /// @param loop 事件循环
    /// @param sock sock
    Connection(EventLoop* loop, std::unique_ptr<Socket> sock);
    

    /// @brief 四种事件
    void read_events();
    void write_events();
    void close_events();
    void error_events();


    /// @brief 定时器是否超时, 用于清理空闲Connection
    /// @param val 距上次请求的时间
    /// @return true超时, false不超时
    bool timer_out(time_t val);


    /// @brief 将send交由IO线程执行
    /// @param data 数据首地址
    /// @param size 数据长度
    void send(const char* data, size_t size);

    /// @brief 封装将数据发送至缓冲区和注册写事件的功能
    /// @param message 数据
    void send_a(std::shared_ptr<std::string> message);



    
    /// @brief 设置回调函数, 分别在四种事件处理后调用
    /// @param func 函数对象
    void set_deal_message_callback(std::function<void(Connection_ptr, std::string&)> func);
    void set_send_complete_callback(std::function<void(Connection_ptr)> func);
    void set_close_callback(std::function<void(Connection_ptr)> func);
    void set_error_callback(std::function<void(Connection_ptr)> func);


    /// @brief 获取连接fd
    /// @return fd
    int get_fd() const;


    /// @brief 获取连接ip
    /// @return ip
    std::string get_ip() const;


    /// @brief 获取连接port
    /// @return port
    uint16_t get_port() const;


    ~Connection();

private:

    // 从事件循环
    EventLoop* _M_loop_ptr;

    // Socket及其信息
    std::unique_ptr<Socket> _M_sock_ptr;
    std::unique_ptr<Channel> _M_channel_ptr;

    // 用户缓冲区
    Buffer _M_input_buffer;
    Buffer _M_output_buffer;

    // 表示Tcp连接的状态: true-已连接, false-已断开
    std::atomic_bool _M_is_diconnected; 

    // 最后接收到数据的时间, 每接收到一个报文, 将时间戳更新为当前时间
    TimeStamp _M_ts; 

    // 关闭连接的回调函数, 将回调TcpServer::close_connection
    std::function<void(Connection_ptr)> _M_close_callback;

    // 连接出错的回调函数, 将回调TcpServer::error_connection
    std::function<void(Connection_ptr)> _M_error_callback;

    // 处理客户端报文请求时, 将回调TcpServer::deal_message
    std::function<void(Connection_ptr, std::string&)> _M_deal_message_callback;

    // 数据发送完成后, 将回调TcpServer::send_complete
    std::function<void(Connection_ptr)> _M_send_complete_callback;
};