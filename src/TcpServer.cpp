#include "../include/TcpServer.hpp"


TcpServer::TcpServer(const std::string& ip, const uint16_t port)
{
    _M_acceptor_ptr = new Acceptor(&_M_main_loop, ip, port);
    _M_acceptor_ptr->set_create_connection_callback(
        std::bind(&TcpServer::create_connection, this, std::placeholders::_1));

    _M_main_loop.set_epoll_timeout_callback(std::bind(&TcpServer::epoll_timeout, this, std::placeholders::_1));
}

void TcpServer::start()
{
    _M_main_loop.run();
}

void TcpServer::deal_message(Connection* conn, std::string& message)
{
    if(_M_deal_message_callback)
        _M_deal_message_callback(conn, message);
}

void TcpServer::create_connection(Socket* clnt_sock)
{
    // 创建Connection对象
    Connection* conn = new Connection(&_M_main_loop, clnt_sock);
    conn->set_close_callback(std::bind(&TcpServer::close_connection, this, std::placeholders::_1));
    conn->set_error_callback(std::bind(&TcpServer::error_connection, this, std::placeholders::_1));
    conn->set_send_complete_callback(std::bind(&TcpServer::send_complete, this, std::placeholders::_1));
    conn->set_deal_message_callback(std::bind(&TcpServer::deal_message, this, 
                                                    std::placeholders::_1, std::placeholders::_2));

    // 将连接用map来管理
    _M_connections_map[conn->get_fd()] = conn;

    if(_M_create_connection_callback)
        _M_create_connection_callback(conn);
}

void TcpServer::close_connection(Connection* conn)
{
    if(_M_close_connection_callback)
        _M_close_connection_callback(conn);

    _M_connections_map.erase(conn->get_fd());
    delete conn;
}

void TcpServer::error_connection(Connection* conn)
{
    if(_M_error_connection_callback)
        _M_error_connection_callback(conn);

    _M_connections_map.erase(conn->get_fd());
    delete conn;
}

void TcpServer::send_complete(Connection* conn)
{
    if(_M_send_complete_callback)
        _M_send_complete_callback(conn);
}

void TcpServer::epoll_timeout(EventLoop* loop)
{
    if(_M_epoll_timeout_callback)
        _M_epoll_timeout_callback(loop);
}

TcpServer::~TcpServer()
{
    delete _M_acceptor_ptr;

    // 释放全部的连接
    for(auto& [fd, conn] : _M_connections_map) {
        delete conn;
    }
}

void TcpServer::set_deal_message_callback(std::function<void(Connection*,std::string &message)> func) {
    _M_deal_message_callback = func;
}

void TcpServer::set_create_connection_callback(std::function<void(Connection*)> func) {
    _M_create_connection_callback = func;
}

void TcpServer::set_close_connection_callback(std::function<void(Connection*)> func) {
    _M_close_connection_callback = func;
}

void TcpServer::set_error_connection_callback(std::function<void(Connection*)> func) {
    _M_error_connection_callback = func;
}

void TcpServer::set_send_complete_callback(std::function<void(Connection*)> func) {
    _M_send_complete_callback = func;
}

void TcpServer::set_epoll_timeout_callback(std::function<void(EventLoop*)> func) {
    _M_epoll_timeout_callback = func;
}
