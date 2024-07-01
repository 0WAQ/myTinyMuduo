#pragma once
#include "Epoll.hpp"

class Channel;
class Epoll;

/**
 *  事件循环类
 *  封装了事件循环的过程,内含一个Epoll对象
 */
class EventLoop
{
public:

    EventLoop();


    /**
     * @describe: 封装服务器代码中的事件循环过程
     * @param:    void
     * @return:   void
     * 
     */
    void run();

    /**
     * @describe:  调用成员变量_M_ep的updata_channel
     * @param:     Channel*
     * @return:    void
     */
    void updata_channel(Channel* ch_ptr);


    ~EventLoop();

private:

    Epoll* _M_ep_ptr;
};