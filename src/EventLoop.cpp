#include "../include/EventLoop.hpp"


EventLoop::EventLoop() : _M_ep(new Epoll){

}


void EventLoop::run()
{
    while(true)
    {
        std::vector<Channel*> channels = _M_ep->wait();

        for(auto& ch : channels) {
            ch->handle();
        }
    }
}

void EventLoop::updata_channel(Channel* ch)
{
    _M_ep->updata_channel(ch);
}

EventLoop::~EventLoop() {
    delete _M_ep;
}