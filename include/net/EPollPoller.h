/**
 * 
 * Epoll头文件
 * 
 */
#ifndef EPOLLPOLLER_H
#define EPOLLPOLLER_H

#include <chrono>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <sys/epoll.h>
#include <vector>
#include <unistd.h>

#include "net/Poller.h"

namespace mymuduo {
namespace net {

class Channel;
class EventLoop;

/**
 * @brief 封装EPollPoller
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
     * @brief 分发事件, 调用epoll_wait, 返回发生事件的合集
     * @param activeChannels 发生的事件合集
     */
    TimeStamp poll(ChannelList *activeChannels, std::chrono::milliseconds timeout) override;

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
    
    /**
     * @brief update_channel与remove_channle的底层操作
     */
    void update(int op, Channel *ch);

private:
    using EventList = std::vector<epoll_event>;

    static const int _M_max_events = 16;    // 发生事件的最大数量
    
    int _M_epoll_fd = -1;       // epollfd

    EventList _M_events_arr;    // 发生事件的合集
};

} // namespace net
} // namespace mymuduo

#endif // EPOLLPOLLER_H