#include <iostream>
#include <signal.h>
#include "EchoServer.h"

EchoServer* server;

void stop(int sig)
{
    LOG_INFO("sig: %d\n", sig);

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

    EventLoop loop(true);
    InetAddress addr(argv[1], atoi(argv[2]));
    std::string name{"EchoServer-01"};

    server = new EchoServer(&loop, addr, name);
    server->start();
    loop.loop();

    return 0;
}

