#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <chrono>

// 用于测试没有分割符的报文
void sep_0(int sock_fd, size_t n)
{
    char buf[1024];
    for(int i = 0; i < n; i++)
    {
        memset(buf, 0, sizeof(buf));
        sprintf(buf, "我是%d号超级大帅哥。", i + 1);

        send(sock_fd, buf, strlen(buf), 0);
    }

    for(int i = 0; i < n; i++)
    {
        memset(buf, 0, sizeof(buf));
        recv(sock_fd, buf, sizeof(buf), 0);

        printf("recv: %s\n", buf);
    }
}

// 用于测试报头长4字节
void sep_1(int sock_fd, size_t n)
{
    char buf[1024];
    for(int i = 0; i < n; i++)
    {
        char tmpbuf[1024]; // 临时buf,存放带有报文头的报文内容
        memset(tmpbuf, 0, sizeof(tmpbuf));
        memset(buf, 0, sizeof(buf));

        sprintf(buf, "我是%d号超级大帅哥。", i + 1);

        int len = strlen(buf);      // 计算报文大小
        memcpy(tmpbuf, &len, 4);    // 拼接报文头部
        memcpy(tmpbuf + 4, buf, len);   // 拼接报文内容

        send(sock_fd, tmpbuf, len + 4, 0);

        /// 以下为接收报文

        len = 0;
        recv(sock_fd, &len, 4, 0);

        memset(buf, 0, sizeof(buf));
        recv(sock_fd, buf, len, 0);

        // printf("recv: %s\n", buf);
    }
}

// 用于测试尾部加上"\r\n\r\n"的分割符
void sep_2(int sock_fd, size_t n)
{
    char buf[1024];
    for(int i = 0; i < n; i++)
    {
        char tmpbuf[1024];
        memset(tmpbuf, 0, sizeof(tmpbuf));
        memset(buf, 0, sizeof(buf));

        sprintf(buf, "我是%d号超级大帅哥。", i + 1);

        int len = strlen(buf);
        memcpy(tmpbuf, buf, len);
        memcpy(tmpbuf + len, "\r\n\r\n", 4);

        send(sock_fd, tmpbuf, strlen(tmpbuf), 0);
    }

    std::string buffer;
    for(int i = 0; i < n; i++)
    {
        memset(buf, 0, sizeof(buf));
        recv(sock_fd, buf, sizeof(buf), 0);
        buffer.append(buf);

        memset(buf, 0, sizeof(buf));

        int len = buffer.find("\r\n\r\n");

        memcpy(buf, buffer.c_str(), len);

        buffer.erase(0, len + 4);

        printf("recv: %s\n", buf);
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

    size_t n = 100'0000;

    auto start = std::chrono::steady_clock::now();

    // sep_0(sock_fd, n);
    sep_1(sock_fd, n);
    // sep_2(sock_fd, n);

    auto end = std::chrono::steady_clock::now();

    std::cout << "消耗时间: " 
        << std::chrono::duration_cast<std::chrono::seconds>(end - start).count() 
        << std::endl;

    return 0;
}

