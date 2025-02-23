#include "Poller.h"
#include "Channel.h"

Poller::Poller(EventLoop *loop) : _M_owner_loop(loop) { }

bool Poller::has_channel(Channel *ch) const
{
    auto it = _M_channel_map.find(ch->get_fd());
    return it != _M_channel_map.end() && it->second == ch;
}