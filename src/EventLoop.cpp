#include "../include/EventLoop.hpp"


EventLoop::EventLoop() : _M_ep_ptr(new Epoll){

}


void EventLoop::run()
{
    while(true)
    {
        std::vector<Channel*> channels = _M_ep_ptr->wait();

        for(auto& ch : channels) {
            ch->handle();
        }
    }
}

void EventLoop::updata_channel(Channel* ch_ptr)
{
    _M_ep_ptr->updata_channel(ch_ptr);
}

EventLoop::~EventLoop() {
    delete _M_ep_ptr;
}