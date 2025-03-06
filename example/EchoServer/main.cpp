#include <iostream>
#include <signal.h>
#include "EchoServer.h"

int main(int argc, char* argv[])
{
    if(argc != 4) {
        std::cout << "Usage: ./tcp_epoll <ip> <port> <log_dir>\n";
        std::cout << "Example: ./tcp_epoll 127.0.0.1 5678 ../../log\n";
        return -1;
    }

    mymuduo::Logger* log = mymuduo::Logger::get_instance(argv[3], "EchoServer");
    log->init(mymuduo::Logger::DEBUG);

    mymuduo::EventLoop loop;
    mymuduo::InetAddress addr(argv[1], atoi(argv[2]));
    std::string name{"EchoServer-01"};

    EchoServer *server = new EchoServer(&loop, addr, name);
    server->start();

    loop.run_every(5.0, [](){
        LOG_INFO("run every 5s.\n");
    });

    loop.loop();

    return 0;
}

