/**
 * 
 * Acceptor头文件
 * 
 */
#ifndef ACCEPTOR_H
#define ACCEPTOR_H

#include <memory>
#include "Socket.h"
#include "InetAddress.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Connection.h"


class Acceptor
{
public:

    using CreateConnCallback = std::function<void(std::unique_ptr<Socket>)>;

public:

    /// @brief 初始化loop与服务端监听地址
    /// @param loop 事件循环
    /// @param ip   ip地址
    /// @param port 监听端口
    Acceptor(EventLoop* loop, const std::string& ip, const uint16_t port);


    /// @brief 设置回调函数
    /// @param func 函数对象
    void set_create_connection_callback(CreateConnCallback func);


    /// @brief 读事件被调函数, 在channel中被回调
    void new_connection();


private:

    // 修改声明的顺序, 应该与初始化顺序一致
    EventLoop* _M_loop_ptr;
    Socket _M_serv_sock;
    Channel _M_acceptor_channel;

    // 创建Connection对象的回调函数, 将回调TcpServer::create_connection
    CreateConnCallback _M_create_connection_callback;
};

#endif // ACCEPTOR_H