#include "../include/Epoll.hpp"


Epoll::Epoll()
{
    if((_M_epoll_fd = epoll_create(1)) == -1) {
        printf("epoll_create() failed(%d).\n", errno);
        exit(-1);
    }
}

void Epoll::updata_channel(Channel* ch_ptr)
{
    epoll_event ev;
    ev.data.ptr = ch_ptr;
    ev.events = ch_ptr->get_monitored_events();
    
    // 判断该fd是否已被监听
    if(ch_ptr->get_in_epoll()) 
    {
        // 若被监听, 则修改events
        if(epoll_ctl(_M_epoll_fd, EPOLL_CTL_MOD, ch_ptr->get_fd(), &ev) == -1) {
            std::cerr << "epoll_ctl() failed.\n";
            exit(-1);
        }
    }
    else 
    {
        // 否则, 新增该fd至epfd
        if(epoll_ctl(_M_epoll_fd, EPOLL_CTL_ADD, ch_ptr->get_fd(), &ev) == -1) {
            std::cerr << "epoll_ctl() failed.\n";
            exit(-1);
        }
        ch_ptr->set_in_epoll();
    }
}

std::vector<Channel*> Epoll::wait(int time_out)
{
    std::vector<Channel*> channels;

    bzero(_M_events_arr, sizeof(_M_events_arr));
    int num_fds = epoll_wait(_M_epoll_fd, _M_events_arr, _M_max_events, time_out);

    // error
    if(num_fds < 0) 
    { 
        std::cerr << "epoll_wait() failed\n";
        exit(-1);
    }
    // timeout
    else if(num_fds == 0) { 
        return channels;
    }

    Channel* ch_ptr;
    for(int i = 0; i < num_fds; i++)
    {
        ch_ptr = (Channel*)_M_events_arr[i].data.ptr;
        ch_ptr->set_happened_events(_M_events_arr[i].events);
        channels.emplace_back(ch_ptr);
    }

    return channels;
}


Epoll::~Epoll()
{
    close(_M_epoll_fd);
}