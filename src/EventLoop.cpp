#include "../include/EventLoop.hpp"


EventLoop::EventLoop() : _M_ep(new Epoll){

}


Epoll* EventLoop::get_epoll() {
    return _M_ep;
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


EventLoop::~EventLoop() {
    delete _M_ep;
}