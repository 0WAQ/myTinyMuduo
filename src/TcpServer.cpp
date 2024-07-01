#include "../include/TcpServer.hpp"


TcpServer::TcpServer(const std::string& ip, const uint16_t port)
{
    _M_acceptor_ptr = new Acceptor(&_M_loop, ip, port);
}

void TcpServer::start()
{
    _M_loop.run();
}

TcpServer::~TcpServer()
{

}