#include "net/Poller.h"
#include "net/Channel.h"

using namespace mymuduo;
using namespace mymuduo::net;

Poller::Poller(EventLoop *loop) : _M_owner_loop(loop) { }

bool Poller::has_channel(Channel *ch) const
{
    auto it = _M_channel_map.find(ch->fd());
    return it != _M_channel_map.end() && it->second == ch;
}

