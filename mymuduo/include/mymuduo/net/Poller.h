#ifndef MYMUDUO_NET_POLLER_H
#define MYMUDUO_NET_POLLER_H

#include <chrono>
#include <vector>
#include <unordered_map>

#include "mymuduo/base/Timestamp.h"
#include "mymuduo/base/noncopyable.h"

namespace mymuduo {
namespace net {

class Channel;
class EventLoop;

/**
 * 多路事件分发器: IO复用模块
 */
class Poller : noncopyable {
    using ChannelList = std::vector<Channel*>;

public:
    Poller(EventLoop *loop);

    /**
     * @brief 事件循环可以通过该接口获取默认(epoll)的IO复用接口
     */
    static Poller* new_default_poller(EventLoop *loop);

    /**
     * @brief
     */
    bool has_channel(Channel *ch) const;
    

    virtual Timestamp poll(ChannelList *channels, std::chrono::system_clock::duration timeout = std::chrono::milliseconds::max()) = 0;
    virtual void update_channel(Channel *ch) = 0;
    virtual void remove_channel(Channel *ch) = 0;

    virtual ~Poller() = default;
    
protected:
    using ChannelMap = std::unordered_map<int, Channel*>;    // key: sockfd

    ChannelMap _channel_map;

private:
    EventLoop* _owner_loop;

};

} // namespace net
} // namespace mymuduo

#endif // MYMUDUO_NET_POLLER_H
