/**
 * 
 * 简易的EchoServer实现
 * 
 */
#ifndef ECHOSERVER_H
#define ECHOSERVER_H

#include <TcpServer.h>

/**
 * @brief 业务服务器: Echo
 */
class EchoServer
{
public:

    using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

    /**
     * @brief 初始化EchoServer
     */
    EchoServer(EventLoop *loop, const InetAddress &addr, const std::string &name) :
            loop_(loop), server_(loop, addr, name)
    {
        server_.set_connection_callback(std::bind(&EchoServer::onConnection, this, std::placeholders::_1));
        server_.set_message_callback(std::bind(&EchoServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        
        server_.set_thread_num(3);
    }

    void start() {
        server_.start();
    }

    void onConnection(const TcpConnectionPtr &conn)
    {
        if(conn->connected())
        {
            LOG_INFO("conn UP : %s", conn->peer_address().get_ip_port().c_str());
        }
        else
        {
            LOG_INFO("conn DOWN : %s", conn->peer_address().get_ip_port().c_str());
        }
    }

    void onMessage(const TcpConnectionPtr &conn, Buffer *buf, TimeStamp time)
    {
        std::string msg;
        if(buf->pick_datagram(msg)) {
            conn->send(msg);
        }
        else {
            LOG_WARN("pick_datagram() return false.\n");
        }
        conn->shutdown();
    }

private:

    EventLoop *loop_;
    TcpServer server_;
};

#endif // ECHOSERVER_H