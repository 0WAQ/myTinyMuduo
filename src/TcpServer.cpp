#include "../include/TcpServer.hpp"


TcpServer::TcpServer(const std::string& ip, const uint16_t port)
{
    _M_acceptor_ptr = new Acceptor(&_M_loop, ip, port);
    _M_acceptor_ptr->set_new_connection_callback(
        std::bind(&TcpServer::create_connection, this, std::placeholders::_1));
}

void TcpServer::start()
{
    _M_loop.run();
}

void TcpServer::create_connection(Socket* clnt_sock)
{
    // 创建Connection对象
    Connection* clnt_conn = new Connection(&_M_loop, clnt_sock);

    printf("new connection(fd = %d, ip = %s, port = %d) ok.\n", 
            clnt_conn->get_fd(), clnt_conn->get_ip().c_str(), clnt_conn->get_port());
}

TcpServer::~TcpServer()
{
    delete _M_acceptor_ptr;
}