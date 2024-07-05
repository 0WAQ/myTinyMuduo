#include "../include/EchoServer.hpp"

EchoServer::EchoServer(const std::string& ip, uint16_t port, size_t thread_num) 
        : _M_tcp_server(ip, port, thread_num) 
{
    // 业务需要什么事件,就回调什么事件
    _M_tcp_server.set_deal_message_callback(std::bind(&EchoServer::handle_deal_message, this, 
                                            std::placeholders::_1, std::placeholders::_2));

    _M_tcp_server.set_close_connection_callback(std::bind(&EchoServer::handle_close_connection, this,
                                            std::placeholders::_1));

    _M_tcp_server.set_error_connection_callback(std::bind(&EchoServer::handle_error_connection, this,
                                            std::placeholders::_1));

    _M_tcp_server.set_create_connection_callback(std::bind(&EchoServer::handle_create_connection, this,
                                            std::placeholders::_1));

    _M_tcp_server.set_send_complete_callback(std::bind(&EchoServer::handle_send_complete, this,
                                            std::placeholders::_1));

    _M_tcp_server.set_epoll_timeout_callback(std::bind(&EchoServer::handle_epoll_timeout, this,
                                            std::placeholders::_1));
}

void EchoServer::start() {
    _M_tcp_server.start();
}

void EchoServer::handle_deal_message(Connection* conn, std::string& message)
{
    printf("thread id = %d, deal message().\n", syscall(SYS_gettid));

    // 假设将数据经过计算后             
    message = "reply: " + message;

    // 将报文发送出去
    conn->send(message.data(), message.size());
}

void EchoServer::handle_create_connection(Connection* conn)
{
    printf("thread id = %d, new connection(ip = %s, port = %d).\n", 
            syscall(SYS_gettid), conn->get_ip().c_str(), conn->get_port());
}

void EchoServer::handle_close_connection(Connection* conn) 
{
    printf("thread id = %d, close connection(client ip is %s).\n", 
                    syscall(SYS_gettid), conn->get_ip().c_str());
}

void EchoServer::handle_error_connection(Connection* conn) 
{
    printf("thread id = %d, error connection(client ip is %s).\n", 
                    syscall(SYS_gettid), conn->get_ip().c_str());
}

void EchoServer::handle_send_complete(Connection* conn) 
{
    printf("thread id = %d, error connection(client ip is %s).\n", 
                    syscall(SYS_gettid), conn->get_ip().c_str());
}

void EchoServer::handle_epoll_timeout(EventLoop* loop) 
{
    printf("thread id = %d, epoll_wait() timeout.\n", syscall(SYS_gettid));
}

EchoServer::~EchoServer() {
    
}
