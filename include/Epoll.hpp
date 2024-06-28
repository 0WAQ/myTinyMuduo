#pragma once

#include <iostream>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <strings.h>
#include <sys/epoll.h>
#include <vector>
#include <unistd.h>


/**
 *  package epoll
 */
class Epoll
{
public:

    /**
     *  create epoll_fd
     */
    Epoll();

    /**
     *  bind fd's op to _M_epoll_fd
     */
    void add_fd(int fd, uint32_t op);

    /**
     *  call epoll_wait()
     *  @return: vector of all the epoll_events triggered
     */
    std::vector<epoll_event> wait(int time_out = -1);

    ~Epoll();
private:
    static const int _M_max_events = 100;
    int _M_epoll_fd = -1;
    epoll_event _M_events[_M_max_events];
};