#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <sys/epoll.h>
#include <netinet/tcp.h>    // include TCP_NODELAY
#include <functional>
#include <binders.h>

#include "../include/InetAddress.hpp"
#include "../include/Socket.hpp"
#include "../include/Epoll.hpp"

int main(int argc, char* argv[])
{
    if(argc != 3) {
        std::cout << "Usage: ./tcp_epoll <ip> <port>\n";
        std::cout << "Example: ./tcp_epoll 127.0.0.1 5678\n";
        return -1;
    }

    // create serv_sock
    Socket serv_sock(create_non_blocking_fd());

    // set serv_sock's opt
    serv_sock.set_keep_alive(true);
    serv_sock.set_reuse_addr(true);
    serv_sock.set_reuse_port(true);
    serv_sock.set_tcp_nodelay(true);

    InetAddress serv_addr(argv[1], atoi(argv[2]));

    serv_sock.bind(serv_addr);
    serv_sock.listen();

    // create epoll_fd
    Epoll ep;

    // 使用serv_channel将serv_fd和ep绑定在一起
    Channel* serv_channel = new Channel(&ep, serv_sock.get_fd());
    // 添加读事件, 并且监听
    serv_channel->set_read_events();
    // 设置serv_channel的执行函数为new_connection
    serv_channel->set_read_callback(std::bind(&Channel::new_connection, serv_channel, &serv_sock));
    
    while(true)
    {
        std::vector<Channel*> channels = ep.wait();

        for(auto& ch : channels) {
            ch->handle();
        }
    }

    return 0;
}

