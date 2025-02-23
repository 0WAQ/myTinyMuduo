/**
 * 
 * 
 * 
 */
#ifndef POLLER_H
#define POLLER_H

#include <vector>
#include <unordered_map>
#include "Channel.h"
#include "EventLoop.h"
#include "TimeStamp.h"
#include "noncopyable.h"

/**
 * 多路事件分发器: IO复用模块
 */
class Poller : noncopyable
{
public:
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop *loop);

    /**
     * @brief 事件循环可以通过该接口获取默认(epoll)的IO复用接口
     */
    static Poller* new_default_poller(EventLoop *loop);

    /**
     * @brief
     */
    bool has_channel(Channel *ch) const;
    

    virtual ~Poller() = default;
    virtual TimeStamp poll(int timeout, ChannelList *activeChannels) = 0;
    virtual void update_channel(Channel *ch) = 0;
    virtual void remove_channel(Channel *ch) = 0;    

protected:
    using ChannelMap = std::unordered_map<int, Channel*>;    // key: sockfd

    ChannelMap _M_channels;

private:

    EventLoop* _M_owner_loop;

};

#endif // POLLER_H