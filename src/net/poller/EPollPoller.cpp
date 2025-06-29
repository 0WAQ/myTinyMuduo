#include "base/Logger.h"
#include "net/Poller.h"
#include "net/Channel.h"
#include "net/poller/EPollPoller.h"

#include <chrono>

using namespace mymuduo;
using namespace mymuduo::net;

EPollPoller::EPollPoller(EventLoop* loop) : 
        Poller(loop),
        _M_epoll_fd(::epoll_create1(EPOLL_CLOEXEC)) ,
        _M_events_arr(_M_max_events)
{
    if(_M_epoll_fd < 0) {
        LOG_ERROR("%s:%s:%d epoll_create() failed: %d.\n", 
            __FILE__, __FUNCTION__, __LINE__, errno);
    }
}

TimeStamp EPollPoller::poll(ChannelList *activeChannels, std::chrono::system_clock::duration timeout)
{
    LOG_DEBUG("func:%s => fd total count=%d\n", __FUNCTION__, activeChannels->size());

    TimeStamp now = TimeStamp::now();
    int numEvents = ::epoll_wait(_M_epoll_fd
                        , _M_events_arr.data()
                        , static_cast<int>(_M_events_arr.size())
                        , timeout == std::chrono::system_clock::duration::max() ? -1 : timeout.count());    
    int savedErrno = errno;  // errno为全局

    if(numEvents > 0) 
    {
        LOG_DEBUG("%d events happened.\n", numEvents);
        
        // 通过epoll_event中的data获取channel指针(在add channel时设置)
        fill_active_channels(numEvents, activeChannels);

        // 实际返回的numEvents和能够容纳的事件数相同, 那么扩容为原来的2倍
        // 采用的是LT模式, 即使该次容量不够导致没有上报, 之后也会调用
        if(numEvents == _M_events_arr.size() * 2) {
            _M_events_arr.resize(_M_events_arr.size() * 2);
        }
    }
    else if(numEvents < 0)
    {
        // MARK:
        // 使用gdb调试时, 会在断点处插入一条普通的中断指令(并且errno也不会被置为0), 
        // 程序在执行到断点处时就会触发一个SIGTRAP信号;
        // 对于一些会阻塞的函数, 如果阻塞的话, 可能会收到gdb调试器发送的信号, 故不返回0值
        // 为解决这个问题, 当epoll_wait返回-1时, 需忽略由于接受调试信号而产生的"错误"返回.
        if(savedErrno != EINTR) {
            errno = savedErrno;
            LOG_ERROR("%s:%s():%d epoll_wait() failed, errno:%d.\n", 
                __FILE__, __FUNCTION__, __LINE__, errno);
        }
    }
    else {
        LOG_DEBUG("%s timeout!\n", __FUNCTION__);
    }

    return now;
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
    channelStatus status = ch->get_status();

    LOG_INFO("func:%s => fd=%d events=%d status=%d\n", __FUNCTION__, ch->fd(), ch->get_happened_events(), status);

    if(status == kNew || status == kDeleted) // 未注册或者已注册未监听
    {
        int fd = ch->fd();

        if(status == kNew) { // 若为新的channel, 将其注册到map中
            _M_channel_map[fd] = ch;
        }
        else { // status == kDeleted, 即channel已注册, 但未被监听
            assert(_M_channel_map.find(fd) != _M_channel_map.end());
            assert(_M_channel_map[fd] == ch);
        }

        ch->set_status(kAdded);
        update(EPOLL_CTL_ADD, ch);  // 监听ch
    }
    else  // 已被监听, 那么要修改
    {
        int fd = ch->fd();

        if(ch->is_none_events()) {  // ch已被监听, 但要将其取消监听
            update(EPOLL_CTL_DEL, ch);
            ch->set_status(kDeleted);
        }
        else {
            update(EPOLL_CTL_MOD, ch);
        }
    }
}

void EPollPoller::remove_channel(Channel* ch)
{
    channelStatus status = ch->get_status();

    LOG_INFO("func:%s => fd=%d events=%d status=%d\n", __FUNCTION__, ch->fd(), ch->get_happened_events(), status);

    // 若channel已经被监听, 那么删除
    if(status == kAdded) {
        update(EPOLL_CTL_DEL, ch);
    }

    // 将其取消注册
    _M_channel_map.erase(ch->fd());
    ch->set_status(kNew);
}

void EPollPoller::update(int op, Channel* ch)
{
    epoll_event ev;
    bzero(&ev, sizeof(epoll_event));

    ev.data.ptr = ch;
    ev.events = ch->get_monitored_events();

    if(epoll_ctl(_M_epoll_fd, op, ch->fd(), &ev) == -1) {
        switch (op)
        {
        case EPOLL_CTL_ADD:
            LOG_ERROR("%s:%s:%d epoll_ctl() add failed: %d.\n", 
                    __FILE__, __FUNCTION__, __LINE__, errno);
            break;

        case EPOLL_CTL_MOD:
            LOG_ERROR("%s:%s:%d epoll_ctl() modify failed: %d.\n", 
                    __FILE__, __FUNCTION__, __LINE__, errno);
            break;
        
        case EPOLL_CTL_DEL:
            LOG_ERROR("%s:%s:%d epoll_ctl() remove failed: %d.\n", 
                    __FILE__, __FUNCTION__, __LINE__, errno);
            break;
        }
    }
}

EPollPoller::~EPollPoller() {
    if (::close(_M_epoll_fd) < 0) {
        LOG_ERROR("%s:%s:%d close error:%d.\n",
            __FILE__, __FUNCTION__, __LINE__, errno);
    }
}

