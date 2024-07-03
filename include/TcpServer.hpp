#pragma once

#include <map>
#include "Socket.hpp"
#include "Channel.hpp"
#include "EventLoop.hpp"
#include "Acceptor.hpp"
#include "Connection.hpp"

/**
 *  
 * 封装监听Socket的创建和事件循环
 * 
 */
class TcpServer
{
public:

    /**
     * 
     * @describe: 构造函数, 用来初始化Socket
     * @param:    要绑定的ip和端口
     * 
     */
    TcpServer(const std::string& ip, const uint16_t port);


    /**
     * 
     * @describe: 转调用了成员变量_M_loop的run方法
     * @param:    void
     * @return:   void
     * 
     */
    void start();


    /**
     * 
     * @describe: 处理客户端报文请求时, Connection回调的函数
     * @param:    Connection*, std::string&
     * @return:   void
     * 
     */
    void deal_message(Connection* conn, std::string& message);


    /**
     * 
     * @describe: 封装处理新的连接请求的代码
     * @param:    Socket*
     * @return:   void
     * 
     */
    void create_connection(Socket* clnt_sock);


    /**
     * 
     * @describe: Tcp连接断开后, Connection回调的函数
     * @param:    void
     * @return:   void
     */
    void close_connection(Connection* conn);


    /**
     * 
     * @describe: Tcp连接出错后, Connection回调的函数
     * @param:    void
     * @return:   void
     */
    void error_connection(Connection* conn);


    /**
     * 
     * @describe: 数据发送完成后, 在Connection类中回调此函数
     * @param:    void
     * @return:   void
     * 
     */
    void send_complete(Connection* conn);


    ~TcpServer();

private:
    EventLoop _M_loop;         // 事件循环变量, 用start方法开始
    Acceptor* _M_acceptor_ptr; // 用于创建监听sock
    std::map<int, Connection*> _M_connections_map;
};