/**
 * 
 * Epoll头文件
 * 
 */
#ifndef EPOLLPOLLER_H
#define EPOLLPOLLER_H

#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <sys/epoll.h>
#include <vector>
#include <unistd.h>

#include "Poller.h"

class Channel;
class EventLoop;

/**
 * @brief 封装Epoll
 */
class EPollPoller : public Poller
{
    using ChannelList = std::vector<Channel*>;

public:

    /**
     * @brief 调用epoll_create, 初始化epfd
     */
    EPollPoller(EventLoop *loop);

    /**
     * @brief 调用epoll_wait, 返回发生事件的合集
     * @return 发生的事件合集
     */
    TimeStamp poll(ChannelList *channels, int timeout = -1) override;

    /**
     * @brief 调用epoll_ctl, 修改ch监听事件
     */
    void update_channel(Channel *ch) override;

    /**
     * @brief 取消监听ch的事件
     */
    void remove_channel(Channel *ch) override;

    ~EPollPoller() override;

private:

    /**
     * @brief 填充活跃的channel
     */
    void fill_active_channels(int numEvents, ChannelList *activeChannels) const;
    
    void update(int op, Channel *ch);

private:
    using EventList = std::vector<epoll_event>;

    // 发生事件的最大数量
    static const int _M_max_events = 16;

    // epfd
    int _M_epoll_fd = -1;

    // 发生事件的合集
    EventList _M_events_arr;
};

#endif // EPOLLPOLLER_H