#include "../include/Epoll.h"

Epoll::Epoll()
{
    if((_M_epoll_fd = epoll_create(1)) == -1) {
        LOG_ERROR("%s:%s:%d epoll_create() failed: %d.\n", 
            __FILE__, __FUNCTION__, __LINE__, errno);
        exit(-1);
    }
}

void Epoll::updata_channel(Channel* ch)
{
    epoll_event ev;
    ev.data.ptr = ch;
    ev.events = ch->get_monitored_events();
    
    // 判断该fd是否已被监听
    if(ch->in_epoll()) 
    {
        // 若被监听, 则修改events
        if(epoll_ctl(_M_epoll_fd, EPOLL_CTL_MOD, ch->get_fd(), &ev) == -1) {
            LOG_ERROR("%s:%s:%d epoll_ctl() modify failed: %d.\n", 
                __FILE__, __FUNCTION__, __LINE__, errno);
            exit(-1);
        }
    }
    else 
    {
        // 否则, 新增该fd至epfd
        if(epoll_ctl(_M_epoll_fd, EPOLL_CTL_ADD, ch->get_fd(), &ev) == -1) {
            LOG_ERROR("%s:%s:%d epoll_ctl() add failed: %d.\n", 
                __FILE__, __FUNCTION__, __LINE__, errno);
            exit(-1);
        }
        ch->set_in_epoll();
    }
}

void Epoll::remove_channel(Channel* ch)
{
    // 若channel已经被监听, 那么删除
    if(ch->in_epoll()) 
    {
        if(epoll_ctl(_M_epoll_fd, EPOLL_CTL_DEL, ch->get_fd(), 0) == -1) {
            LOG_ERROR("%s:%s:%d epoll_ctl() remove failed: %d.\n", 
                __FILE__, __FUNCTION__, __LINE__, errno);
            exit(-1);
        }
    }
}

std::vector<Channel*> Epoll::wait(int time_out)
{
    std::vector<Channel*> channels;

    bzero(_M_events_arr, sizeof(_M_events_arr));
    int num_fds = epoll_wait(_M_epoll_fd, _M_events_arr, _M_max_events, time_out);


    if(num_fds < 0) 
    { 
        LOG_ERROR("%s:%s:%d epoll_wait() failed: %d.\n", 
            __FILE__, __FUNCTION__, __LINE__, errno);
        exit(-1);
    }
    else if(num_fds == 0) { 
        return channels;
    }

    Channel* ch;
    for(int i = 0; i < num_fds; i++)
    {
        ch = (Channel*)_M_events_arr[i].data.ptr;
        ch->set_happened_events(_M_events_arr[i].events);
        channels.emplace_back(ch);
    }

    return channels;
}

Epoll::~Epoll() { close(_M_epoll_fd);}