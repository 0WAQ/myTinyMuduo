#include "Poller.h"
#include "EPollPoller.h"
#include "Channel.h"
#include "Logger.h"

const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;

EPollPoller::EPollPoller(EventLoop* loop) : 
        Poller(loop),
        _M_epoll_fd(::epoll_create1(EPOLL_CLOEXEC)) ,
        _M_events_arr(_M_max_events)
{
    if(_M_epoll_fd < 0) {
        LOG_ERROR("%s:%s:%d epoll_create() failed: %d.\n", 
            __FILE__, __FUNCTION__, __LINE__, errno);
        exit(-1);
    }
}

TimeStamp EPollPoller::poll(ChannelList *channels, int timeout)
{
    int numEvents = ::epoll_wait(_M_epoll_fd, &*_M_events_arr.begin(), 
                                    static_cast<int>(_M_events_arr.size()), timeout);
    int saveErrno = errno;

    if(numEvents < 0) 
    {
        if(saveErrno != EINTR) {
            LOG_ERROR("%s:%s():%d epoll_wait() failed: %d.\n", 
                __FILE__, __FUNCTION__, __LINE__, errno);
            exit(-1);
        }
    }
    else if(numEvents == 0) { 
        LOG_DEBUG("%s timeout! \n", __FUNCTION__);
    }
    else
    {
        LOG_INFO("%d events happened.\n", numEvents);

        fill_active_channels(numEvents, channels);

        if(numEvents == _M_events_arr.size() * 2) {
            _M_events_arr.resize(_M_events_arr.size() * 2);
        }
    }

    return TimeStamp::now();
}

void EPollPoller::fill_active_channels(int numEvents, ChannelList *activeChannels) const
{
    for(int i = 0; i < numEvents; i++)
    {
        Channel *ch = static_cast<Channel*>(_M_events_arr[i].data.ptr);
        ch->set_happened_events(_M_events_arr[i].events);
        activeChannels->emplace_back(ch);
    }
}

void EPollPoller::update_channel(Channel* ch)
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

void EPollPoller::remove_channel(Channel* ch)
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

EPollPoller::~EPollPoller() {
    ::close(_M_epoll_fd);
}