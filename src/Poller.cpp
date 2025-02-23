#include "Poller.h"
#include "Channel.h"

Poller::Poller(EventLoop *loop) : _M_owner_loop(loop) { }

bool Poller::has_channel(Channel *ch) const
{
    auto it = _M_channels.find(ch->get_fd());
    return it != _M_channels.end() && it->second == ch;
}