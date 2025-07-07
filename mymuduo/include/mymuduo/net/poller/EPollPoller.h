#ifndef MYMUDUO_NET_TIMESTAMP_H
#define MYMUDUO_NET_TIMESTAMP_H

#include <chrono>
#include <vector>
#include <cstring>
#include <sys/epoll.h>

#include "mymuduo/net/Poller.h"

namespace mymuduo {
namespace net {

class Channel;
class EventLoop;

class EPollPoller : public Poller {
public:
    using ChannelList = std::vector<Channel*>;

public:
    EPollPoller(EventLoop *loop);
    ~EPollPoller() override;

    /**
     * @brief 分发事件, 调用epoll_wait, 返回发生事件的合集
     */
    Timestamp poll(ChannelList *activeChannels, std::chrono::system_clock::duration timeout) override;

    /**
     * @brief 调用epoll_ctl, 修改ch监听事件
     */
    void update_channel(Channel *ch) override;

    /**
     * @brief 取消监听ch的事件
     */
    void remove_channel(Channel *ch) override;

private:
    void fill_active_channels(int numEvents, ChannelList *activeChannels) const;
    
    /**
     * @brief update_channel与remove_channle的底层操作
     */
    void update(int op, Channel *ch);

private:
    using EventList = std::vector<epoll_event>;

    static const int _max_events = 16;    // 发生事件的最大数量
    
    int _epoll_fd = -1;       // epollfd

    EventList _events_arr;    // 发生事件的合集
};

} // namespace net
} // namespace mymuduo

#endif // MYMUDUO_NET_TIMESTAMP_H