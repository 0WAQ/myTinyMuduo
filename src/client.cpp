#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <ctime>

int main(int argc, char* argv[])
{
    if(argc != 3) {
        std::cout << "Usage: ./client <ip> <port>.\n";
        std::cout << "Example: ./client 127.0.0.1 5678\n";
        return -1;
    }

    int sock_fd;
    if((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "socket() failed.\n";
        return -1;
    }

    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if(connect(sock_fd, (sockaddr*)&serv_addr, sizeof(serv_addr)) != 0) {
        printf("connect(%s: %s) failed.\n", argv[1], argv[2]);
        close(sock_fd);
        return -1;
    }
    std::cout << "connect ok.\n";

    char buf[1024];
    for(int i = 0; i < 200000; i++)
    {
        memset(buf, 0, sizeof(buf));
        std::cout << "please input: ";
        std::cin >> buf;

        if(send(sock_fd, buf, strlen(buf), 0) <= 0) {
            std::cerr << "write() failed.\n";
            close(sock_fd);
            return -1;
        }

        memset(buf, 0, sizeof(buf));
        if(recv(sock_fd, buf, sizeof(buf), 0) <= 0) {
            std::cerr << "read() failed.\n";
            close(sock_fd);
            return -1;
        }

        printf("recv:%s\n", buf);
    }

    return 0;
}