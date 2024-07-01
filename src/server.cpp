#include "../include/TcpServer.hpp"

int main(int argc, char* argv[])
{
    if(argc != 3) {
        std::cout << "Usage: ./tcp_epoll <ip> <port>\n";
        std::cout << "Example: ./tcp_epoll 127.0.0.1 5678\n";
        return -1;
    }

    TcpServer tcp_server(argv[1], atoi(argv[2]));
    
    tcp_server.start();

    return 0;
}

