#include "../include/TcpServer.hpp"


TcpServer::TcpServer(const std::string& ip, const uint16_t port)
{
    _M_acceptor_ptr = new Acceptor(&_M_loop, ip, port);
    _M_acceptor_ptr->set_create_connection_callback(
        std::bind(&TcpServer::create_connection, this, std::placeholders::_1));
}

void TcpServer::start()
{
    _M_loop.run();
}

void TcpServer::create_connection(Socket* clnt_sock)
{
    // 创建Connection对象
    Connection* conn = new Connection(&_M_loop, clnt_sock);

    printf("new connection(fd = %d, ip = %s, port = %d) ok.\n", 
            conn->get_fd(), conn->get_ip().c_str(), conn->get_port());

    // 将连接用map来管理
    _M_connections_map[conn->get_fd()] = conn;
}

TcpServer::~TcpServer()
{
    delete _M_acceptor_ptr;

    // 释放全部的连接
    for(auto& [fd, conn] : _M_connections_map) {
        delete conn;
    }
}