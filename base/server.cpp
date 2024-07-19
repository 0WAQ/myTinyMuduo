#include <iostream>
#include <signal.h>
#include "../include/EchoServer.hpp"
#include "../include/Logger.hpp"

EchoServer* server;

void stop(int sig)
{
    LOG_INFO("sig: %d\n", sig);

    // 调用EchoServer::stop函数停止服务
    server->stop();

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

    Logger* log = Logger::get_instance();
    log->set_level(LogLevel::DEBUG);

    server = new EchoServer(argv[1], atoi(argv[2]), 1, 0);
    server->start();

    return 0;
}

