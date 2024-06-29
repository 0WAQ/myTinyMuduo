#include "../include/Channel.hpp"

Channel::Channel(Epoll* ep, int fd) : _M_ep(ep), _M_fd(fd)
{

}

int Channel::get_fd() {
    return _M_fd;
}

void Channel::set_ET() {
    _M_monitored_events |= EPOLLET;
}

// 出了设置为读事件, 还直接调用updata_channel去添加
void Channel::set_read_events() {
    _M_monitored_events |= EPOLLIN;
    _M_ep->updata_channel(this);
}

void Channel::set_in_epoll() {
    _M_in_epoll = true;
}

bool Channel::get_in_epoll() {
    return _M_in_epoll;
}

void Channel::set_happened_events(uint32_t events) {
    _M_happened_events = events;
}

uint32_t Channel::get_happened_events() {
    return _M_happened_events;
}

uint32_t Channel::get_monitored_events() {
    return _M_monitored_events;
}

Channel::~Channel()
{

}