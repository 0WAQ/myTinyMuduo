#include <iostream>
#include <signal.h>
#include "../include/EchoServer.hpp"

EchoServer* server;

void stop(int sig)
{
    std::cout << "sig: " << sig << std::endl;

    // 调用EchoServer::stop函数停止服务
    server->stop();

    std::cout << "echo-server已停止\n" << fflush;

    delete server;
    exit(0);
}

int main(int argc, char* argv[])
{
    if(argc != 3) {
        std::cout << "Usage: ./tcp_epoll <ip> <port>\n";
        std::cout << "Example: ./tcp_epoll 127.0.0.1 5678\n";
        return -1;
    }

    signal(SIGTERM, stop);
    signal(SIGINT, stop);

    server = new EchoServer(argv[1], atoi(argv[2]), 10, 0);
    server->start();

    return 0;
}

