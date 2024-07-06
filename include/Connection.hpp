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

class Connection;
using Connection_ptr = std::shared_ptr<Connection>;

/**
 *  Channel之上的封装类, 专门用于创建客户端的Socket
 *  继承自这个类, 用shared_from_this代替this指针
 */
class Connection : public std::enable_shared_from_this<Connection>
{
public:

    /**
     * 
     * @describe: 初始化loop与clnt_sock
     * @param:    EventLoop*, Socket*
     * 
     */
    Connection(EventLoop* loop, Socket* clnt_sock);
    
    
    /**
     * 
     * @describe: 获取fd
     * @param:    void
     * @return:   int
     * 
     */
    int get_fd();


    /**
     * 
     * @describe: 获取ip
     * @param:    void
     * @return:   uint16_t
     */
    std::string get_ip() const;


    /**
     * 
     * @describe: 获取port
     * @param:    void
     * @return:   uint16_t
     * 
     */
    uint16_t get_port() const;


    /**
     * 
     * @describe: Tcp连接断开后, Channel回调的函数
     * @param:    void
     * @return:   void
     */
    void close_callback();


    /**
     * 
     * @describe: Tcp连接出错后, Channel回调的函数
     * @param:    void
     * @return:   void
     */
    void error_callback();

    
    /**
     * 
     * @describe: 设置回调函数
     * @param:    std::function<void(Connection_ptr)>
     * @return :  void
     * 
     */
    void set_close_callback(std::function<void(Connection_ptr)> func);
    void set_error_callback(std::function<void(Connection_ptr)> func);
    void set_send_complete_callback(std::function<void(Connection_ptr)> func);

    /**
     * @extra_param: std::string&
     */
    void set_deal_message_callback(std::function<void(Connection_ptr, std::string&)> func);


    /**
     * 
     * @describe: 封装处理新消息的代码, 有新消息时, 被Channel的读事件回调
     * @param:    void
     * @return:   void
     * 
     */
    void new_message();


    /**
     * 
     * @describe: 写事件发生时, Channel回调的函数
     * @param:    void
     * @return:   void
     * 
     */
    void write_events();
    

    /**
     * 
     * @describe: 封装了输出到用户缓冲区的功能
     * @param:    const char*, size_t
     * @return:   void
     */
    void send(const char* data, size_t size);


    ~Connection();

private:
    EventLoop* _M_loop_ptr;
    Socket* _M_clnt_sock_ptr;
    Channel* _M_clnt_channel_ptr;
    std::atomic_bool _M_is_discon; // 用于判断当前连接是否断开

    // 用户缓冲区
    Buffer _M_input_buffer;
    Buffer _M_output_buffer;

    // 关闭连接的回调函数, 将回调TcpServer::close_connection()
    std::function<void(Connection_ptr)> _M_close_callback;

    // 连接出错的回调函数, 将回调TcpServer::error_connection()
    std::function<void(Connection_ptr)> _M_error_callback;

    // 处理客户端报文请求时, 将回调TcpServer::deal_message()
    std::function<void(Connection_ptr, std::string&)> _M_deal_message_callback;

    // 数据发送完成后, 将回调TcpServer::send_complete(Connection_ptr)
    std::function<void(Connection_ptr)> _M_send_complete_callback;
};