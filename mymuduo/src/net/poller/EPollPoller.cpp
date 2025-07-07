#include "mymuduo/base/Logger.h"
#include "mymuduo/net/Poller.h"
#include "mymuduo/net/Channel.h"
#include "mymuduo/net/poller/EPollPoller.h"

#include <cassert>
#include <cerrno>
#include <chrono>
#include <cstring>

using namespace mymuduo;
using namespace mymuduo::net;

EPollPoller::EPollPoller(EventLoop* loop) : 
        Poller(loop),
        _epoll_fd(::epoll_create1(EPOLL_CLOEXEC)) ,
        _events_arr(_max_events)
{
    if(_epoll_fd < 0) {
        LOG_ERROR("%s:%s:%d - errno = %d %s.\n", 
            __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
    }
}

Timestamp EPollPoller::poll(ChannelList *activeChannels, std::chrono::system_clock::duration timeout)
{
    using namespace std::chrono;

    LOG_DEBUG("func:%s => fd total count=%d\n", __FUNCTION__, activeChannels->size());

    Timestamp now = Timestamp::now();
    int numEvents = ::epoll_wait(_epoll_fd, _events_arr.data()
                        , static_cast<int>(_events_arr.size())
                        , timeout == system_clock::duration::max() 
                                    ? -1 : duration_cast<milliseconds>(timeout).count());    
    int savedErrno = errno;  // errno为全局

    if(numEvents > 0) 
    {
        LOG_DEBUG("%d events happened.\n", numEvents);
        
        // 通过epoll_event中的data获取channel指针(在add channel时设置)
        fill_active_channels(numEvents, activeChannels);

        // 实际返回的numEvents和能够容纳的事件数相同, 那么扩容为原来的2倍
        // 采用的是LT模式, 即使该次容量不够导致没有上报, 之后也会调用
        if(numEvents == _events_arr.size() * 2) {
            _events_arr.resize(_events_arr.size() * 2);
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
            LOG_ERROR("%s:%s:%d - errno = %d %s.\n", 
                __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
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
        Channel *ch = static_cast<Channel*>(_events_arr[i].data.ptr);
        ch->set_happened_events(_events_arr[i].events);
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
            _channel_map[fd] = ch;
        }
        else { // status == kDeleted, 即channel已注册, 但未被监听
            assert(_channel_map.find(fd) != _channel_map.end());
            assert(_channel_map[fd] == ch);
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
    _channel_map.erase(ch->fd());
    ch->set_status(kNew);
}

void EPollPoller::update(int op, Channel* ch)
{
    epoll_event ev;
    bzero(&ev, sizeof(epoll_event));

    ev.data.ptr = ch;
    ev.events = ch->get_monitored_events();

    if(epoll_ctl(_epoll_fd, op, ch->fd(), &ev) == -1) {
        switch (op)
        {
        case EPOLL_CTL_ADD:
            LOG_ERROR("%s:%s:%d epoll_ctl() add failed - errno = %d %s.\n", 
                __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
            break;

        case EPOLL_CTL_MOD:
            LOG_ERROR("%s:%s:%d epoll_ctl() modify failed - errno = %d %s.\n", 
                __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
            break;
        
        case EPOLL_CTL_DEL:
            LOG_ERROR("%s:%s:%d epoll_ctl() remove failed - errno = %d %s.\n", 
                __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
            break;
        }
    }
}

EPollPoller::~EPollPoller() {
    if (::close(_epoll_fd) < 0) {
        LOG_ERROR("%s:%s:%d - errno = %d %s.\n", 
            __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
    }
}

