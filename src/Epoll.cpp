#include "../include/Epoll.hpp"


Epoll::Epoll()
{
    if((_M_epoll_fd = epoll_create(1)) == -1) {
        printf("epoll_create() failed(%d).\n", errno);
        exit(-1);
    }
}

void Epoll::add_fd(int fd, uint32_t op)
{
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = op;

    if(::epoll_ctl(_M_epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        printf("epoll_ctl() failed(%d).\n", errno);
        exit(-1);
    }
}

std::vector<epoll_event> Epoll::wait(int time_out)
{
    std::vector<epoll_event> evs;

    bzero(_M_events, sizeof(_M_events));

    int num_fds = epoll_wait(_M_epoll_fd, _M_events, _M_max_events, time_out);

        // error
    if(num_fds < 0) 
    { 
        std::cerr << "epoll_wait() failed\n";
        exit(-1);
    }
    // timeout
    else if(num_fds == 0) 
    { 
        std::cout << "epoll_wait() timeout.\n";
        return evs;
    }

    for(int i = 0; i < num_fds; i++) {
        evs.push_back(_M_events[i]);
    }

    return evs;
}


Epoll::~Epoll()
{
    close(_M_epoll_fd);
}