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

void set_non_blocking(int fd) {
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
}

int main(int argc, char* argv[])
{
    if(argc != 3) {
        std::cout << "Usage: ./tcp_epoll <ip> <port>\n";
        std::cout << "Example: ./tcp_epoll 127.0.0.1 5678\n";
        return -1;
    }

    // create listen_fd
    int listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(listen_fd < 0) {
        std::cerr << "socket() failed\n";
        return -1;
    }

    // set listen_fd's opt
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, static_cast<socklen_t>(sizeof(opt)));
    setsockopt(listen_fd, SOL_SOCKET, TCP_NODELAY,  &opt, static_cast<socklen_t>(sizeof(opt)));
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEPORT, &opt, static_cast<socklen_t>(sizeof(opt)));
    setsockopt(listen_fd, SOL_SOCKET, SO_KEEPALIVE, &opt, static_cast<socklen_t>(sizeof(opt)));
    set_non_blocking(listen_fd); // set non-blocking IO

    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if(bind(listen_fd, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "bind() failed\n";
        close(listen_fd);
        return -1;
    }

    if(listen(listen_fd, 128) != 0) {
        std::cerr << "listen() failed\n";
        close(listen_fd);
        return -1;
    }

    // create epoll_fd
    int epoll_fd = epoll_create(1);

    // create a struct monitored listen_fd's read events
    epoll_event ev;
    ev.data.fd = listen_fd;
    ev.events = EPOLLIN;

    // add listen_fd to epoll_fd binds ev
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev);
    epoll_event evs[10];

    while(true)
    {
        int num_fds = epoll_wait(epoll_fd, evs, 10, -1);
        
        if(num_fds < 0) { // error
            std::cerr << "epoll_wait() failed\n";
            break;
        }
        else if(num_fds == 0) { // timeout
            std::cout << "epoll_wait() timeout.\n";
            continue;
        }
        
        for(int i = 0; i < num_fds; i++) 
        {   
            // new connect
            if(evs[i].data.fd == listen_fd)
            {
                ///////////////////////////////////////////
                sockaddr_in clnt_addr;
                socklen_t clnt_addr_len = sizeof(clnt_addr);

                int clnt_fd = accept(listen_fd, (sockaddr*)&clnt_addr, &clnt_addr_len);
                set_non_blocking(clnt_fd);

                printf("Accept client(fd = %d, ip = %s, port = %d) ok.\n", 
                            clnt_fd, inet_ntoa(clnt_addr.sin_addr), ntohs(clnt_addr.sin_port));

                
                // add clnt_fd to epoll_fd binds ev
                ev.data.fd = clnt_fd;
                ev.events = EPOLLIN | EPOLLET; // Edge Triggle
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, clnt_fd, &ev);
                ///////////////////////////////////////////
            }
            else
            {
                ///////////////////////////////////////////
                if(evs[i].events & EPOLLRDHUP) { // close events
                    printf("1.client(clnt_fd = %d) disconnected\n", evs[i].data.fd);
                    close(evs[i].data.fd);
                }
                else if(evs[i].events & (EPOLLIN | EPOLLPRI)) // read events
                {
                    char buf[1024];

                    while(true) // non-blocking IO
                    {
                        // init all buf as 0
                        bzero(&buf, sizeof(buf));

                        ssize_t nread = read(evs[i].data.fd, buf, sizeof(buf));
                        // read datas successfully
                        if(nread > 0) {
                            printf("recv(clnt_fd = %d): %s\n", evs[i].data.fd, buf);
                            send(evs[i].data.fd, buf, strlen(buf), 0);
                        }
                        // read failed because interrupted by system call
                        else if(nread == -1 && errno == EINTR) {
                            continue;
                        }
                        // read failed because datas've been read
                        else if(nread == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                            break;
                        }
                        // clnt has been disconnected
                        else if(nread == 0) {
                            printf("2.client(clnt_fd = %d) disconnected.\n", evs[i].data.fd);
                            close(evs[i].data.fd);
                            break;
                        }
                    }
                }
                else if(evs[i].events & EPOLLOUT) { // write events
                    
                }
                else {
                    printf("client(clnt_fd = %d) error.\n", evs[i].data.fd);
                    close(evs[i].data.fd);
                }
                ///////////////////////////////////////////
            }
        }
    }

    return 0;
}

