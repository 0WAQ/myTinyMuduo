#include "../include/TcpServer.hpp"


TcpServer::TcpServer(const std::string& ip, const uint16_t port)
{
    // create serv_sock
    Socket* serv_sock_ptr = new Socket(create_non_blocking_fd());

    // set serv_sock's opt
    serv_sock_ptr->set_keep_alive(true);
    serv_sock_ptr->set_reuse_addr(true);
    serv_sock_ptr->set_reuse_port(true);
    serv_sock_ptr->set_tcp_nodelay(true);

    InetAddress serv_addr(ip, port);

    serv_sock_ptr->bind(serv_addr);
    serv_sock_ptr->listen();


    // 使用serv_channel将serv_fd和ep绑定在一起
    Channel* serv_channel = new Channel(&_M_loop, serv_sock_ptr->get_fd());
    // 添加读事件, 并且监听
    serv_channel->set_read_events();
    // 设置serv_channel的执行函数为new_connection
    serv_channel->set_read_callback(std::bind(&Channel::new_connection, serv_channel, serv_sock_ptr));

}

void TcpServer::start()
{
    _M_loop.run();
}

TcpServer::~TcpServer()
{

}