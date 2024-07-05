#include "../include/EventLoop.hpp"

EventLoop::EventLoop() : _M_ep_ptr(new Epoll){

}

void EventLoop::run()
{
    while(true)
    {
        std::vector<Channel*> channels = _M_ep_ptr->wait(10*1000);

        // 若channels为空, 则说明超时, 通知TcpServer
        if(channels.empty()) {
            if(_M_epoll_wait_timeout_callback)
                _M_epoll_wait_timeout_callback(this);
        }
        else {
            for(auto& ch : channels) {
                ch->handle();
            }
        }
    }
}

void EventLoop::updata_channel(Channel* ch_ptr)
{
    _M_ep_ptr->updata_channel(ch_ptr);
}

void EventLoop::set_epoll_timeout_callback(std::function<void(EventLoop*)> func) {
    _M_epoll_wait_timeout_callback = func;
}

EventLoop::~EventLoop() {
    delete _M_ep_ptr;
}