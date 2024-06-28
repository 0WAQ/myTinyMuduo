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

#include "../include/InetAddress.hpp"
#include "../include/Socket.hpp"
#include "../include/Epoll.hpp"

int main(int argc, char* argv[])
{
    if(argc != 3) {
        std::cout << "Usage: ./tcp_epoll <ip> <port>\n";
        std::cout << "Example: ./tcp_epoll 127.0.0.1 5678\n";
        return -1;
    }

    // create serv_sock
    Socket serv_sock(create_non_blocking_fd());

    // set serv_sock's opt
    serv_sock.set_keep_alive(true);
    serv_sock.set_reuse_addr(true);
    serv_sock.set_reuse_port(true);
    serv_sock.set_tcp_nodelay(true);

    InetAddress serv_addr(argv[1], atoi(argv[2]));

    serv_sock.bind(serv_addr);
    serv_sock.listen();

    // create epoll_fd
    Epoll ep;

    // add serv_sock to epoll_fd binds ev
    ep.add_fd(serv_sock.get_fd(), EPOLLIN);
    std::vector<epoll_event> evs;

    while(true)
    {
        evs = ep.wait();

        for(auto& ev : evs) 
        {
            // close events
            if(ev.events & EPOLLRDHUP) { 
                printf("client(clnt_fd = %d) disconnected\n", ev.data.fd);
                close(ev.data.fd);
            }
            // read events
            else if(ev.events & (EPOLLIN | EPOLLPRI)) 
            {
                // serv_sock
                if(ev.data.fd == serv_sock.get_fd())
                {
                    InetAddress clnt_addr;
                    Socket* clnt_sock = new Socket(serv_sock.accept(clnt_addr));

                    printf("Accept client(fd = %d, ip = %s, port = %d) ok.\n", 
                                clnt_sock->get_fd(), clnt_addr.ip(), clnt_addr.port());

                    
                    // add clnt_fd to epoll_fd binds ev
                    ep.add_fd(clnt_sock->get_fd(), EPOLLIN | EPOLLET);
                }
                // other fd
                else
                {
                    char buf[1024];
                    while(true) // non-blocking IO
                    {
                        // init all buf as 0
                        bzero(&buf, sizeof(buf));

                        ssize_t nlen = read(ev.data.fd, buf, sizeof(buf));

                        // read datas successfully
                        if(nlen > 0) 
                        {
                            printf("recv(clnt_fd = %d): %s\n", ev.data.fd, buf);
                            send(ev.data.fd, buf, strlen(buf), 0);
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
                            printf("client(clnt_fd = %d) disconnected.\n", ev.data.fd);
                            close(ev.data.fd);
                            break;
                        }
                    }
                }
            }
            // write events
            else if(ev.events & EPOLLOUT) 
            {
                
            }
            // error events
            else 
            {
                printf("client(clnt_fd = %d) error.\n", ev.data.fd);
                close(ev.data.fd);
            }
        }
    }

    return 0;
}

