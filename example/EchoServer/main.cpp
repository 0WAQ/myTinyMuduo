#include <iostream>
#include <signal.h>
#include "EchoServer.h"

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
    if(argc != 4) {
        std::cout << "Usage: ./tcp_epoll <ip> <port> <log_dir>\n";
        std::cout << "Example: ./tcp_epoll 127.0.0.1 5678 ../../log\n";
        return -1;
    }

    signal(SIGTERM, stop);
    signal(SIGINT, stop);

    Logger* log = Logger::get_instance();
    log->init(DEBUG, argv[3], ".log");

    server = new EchoServer(argv[1], atoi(argv[2]), 3, 2);
    server->start();

    return 0;
}

