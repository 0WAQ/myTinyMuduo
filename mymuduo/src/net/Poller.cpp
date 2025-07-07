#include "mymuduo/net/Poller.h"
#include "mymuduo/net/Channel.h"

using namespace mymuduo;
using namespace mymuduo::net;

Poller::Poller(EventLoop *loop) : _owner_loop(loop) { }

bool Poller::has_channel(Channel *ch) const
{
    auto it = _channel_map.find(ch->fd());
    return it != _channel_map.end() && it->second == ch;
}

