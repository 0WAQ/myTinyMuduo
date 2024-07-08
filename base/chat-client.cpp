#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <ctime>
#include <thread>
#include <future>
#include <functional>

std::mutex mutex;

void recv_a(int sock_fd)
{
    while(true)
    {        
        char buf[1024] = {0};
        
        int len = 0;
        recv(sock_fd, &len, 4, 0);
        memset(buf, 0, sizeof(buf));
        recv(sock_fd, buf, len, 0);

        std::lock_guard<std::mutex> gd(mutex);
        std::cout << "recv: " << buf << std::endl;
    }
}


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

    std::thread th(std::bind(&recv_a, sock_fd));

    char buf[1020];
    while(true)
    {
        char tmpbuf[1024]; // 临时buf,存放带有报文头的报文内容
        memset(tmpbuf, 0, sizeof(tmpbuf));
        memset(buf, 0, sizeof(buf));

        {
            std::lock_guard<std::mutex> gd(mutex);
            std::cout << "please input: ";
            std::cin >> buf;
        }

        int len = strlen(buf);      // 计算报文大小
        memcpy(tmpbuf, &len, 4);    // 拼接报文头部
        memcpy(tmpbuf + 4, buf, len);   // 拼接报文内容

        send(sock_fd, tmpbuf, len + 4, 0);
    }
    
    th.join();

    return 0;
}