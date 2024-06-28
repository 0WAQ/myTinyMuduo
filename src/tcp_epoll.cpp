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

#include "InetAddress.hpp"
#include "Socket.hpp"

int main(int argc, char* argv[])
{
    if(argc != 3) {
        std::cout << "Usage: ./tcp_epoll <ip> <port>\n";
        std::cout << "Example: ./tcp_epoll 127.0.0.1 5678\n";
        return -1;
    }

    // create listen_sock
    Socket listen_sock(create_non_blocking_fd());

    // set listen_sock's opt
    listen_sock.set_keep_alive(true);
    listen_sock.set_reuse_addr(true);
    listen_sock.set_reuse_port(true);
    listen_sock.set_tcp_nodelay(true);

    InetAddress serv_addr(argv[1], atoi(argv[2]));

    listen_sock.bind(serv_addr);
    listen_sock.listen();

    // create epoll_fd
    int epoll_fd = epoll_create(1);

    // create a struct monitored listen_sock's read events
    epoll_event ev;
    ev.data.fd = listen_sock.get_fd();
    ev.events = EPOLLIN;

    // add listen_sock to epoll_fd binds ev
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_sock.get_fd(), &ev);
    epoll_event evs[10];

    while(true)
    {
        int num_fds = epoll_wait(epoll_fd, evs, 10, -1);
        
        // error
        if(num_fds < 0) 
        { 
            std::cerr << "epoll_wait() failed\n";
            break;
        }
        // timeout
        else if(num_fds == 0) 
        { 
            std::cout << "epoll_wait() timeout.\n";
            continue;
        }
        
        for(int i = 0; i < num_fds; i++) 
        {
            // close events
            if(evs[i].events & EPOLLRDHUP) { 
                printf("client(clnt_fd = %d) disconnected\n", evs[i].data.fd);
                close(evs[i].data.fd);
            }
            // read events
            else if(evs[i].events & (EPOLLIN | EPOLLPRI)) 
            {
                // listen_sock
                if(evs[i].data.fd == listen_sock.get_fd())
                {
                    ///////////////////////////////////////////
                    InetAddress clnt_addr;
                    Socket* clnt_sock = new Socket(listen_sock.accept(clnt_addr));

                    printf("Accept client(fd = %d, ip = %s, port = %d) ok.\n", 
                                clnt_sock->get_fd(), clnt_addr.ip(), clnt_addr.port());

                    
                    // add clnt_fd to epoll_fd binds ev
                    ev.data.fd = clnt_sock->get_fd();
                    ev.events = EPOLLIN | EPOLLET; // Edge Triggle
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, clnt_sock->get_fd(), &ev);
                    ///////////////////////////////////////////
                }
                // other fd
                else
                {
                    char buf[1024];
                    while(true) // non-blocking IO
                    {
                        // init all buf as 0
                        bzero(&buf, sizeof(buf));

                        ssize_t nlen = read(evs[i].data.fd, buf, sizeof(buf));

                        // read datas successfully
                        if(nlen > 0) 
                        {
                            printf("recv(clnt_fd = %d): %s\n", evs[i].data.fd, buf);
                            send(evs[i].data.fd, buf, strlen(buf), 0);
                        }
                        // read failed because interrupted by system call
                        else if(nlen == -1 && errno == EINTR) 
                        {
                            continue;
                        }
                        // read failed because datas've been read
                        else if(nlen == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) 
                        {
                            break;
                        }
                        // clnt has been disconnected
                        else if(nlen == 0) 
                        {
                            printf("client(clnt_fd = %d) disconnected.\n", evs[i].data.fd);
                            close(evs[i].data.fd);
                            break;
                        }
                    }
                }
            }
            // write events
            else if(evs[i].events & EPOLLOUT) 
            { 
                
            }
            // error events
            else 
            {
                printf("client(clnt_fd = %d) error.\n", evs[i].data.fd);
                close(evs[i].data.fd);
            }
        }
    }

    return 0;
}

