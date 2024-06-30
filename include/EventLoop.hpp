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
     * 
     * @describe: 获取内部的Epoll对象_M_ep
     * @param:    void
     * @return:   Epoll*
     * 
     */
    Epoll* get_epoll();


    /**
     * @describe: 封装服务器代码中的事件循环过程
     * @param:    void
     * @return:   void
     * 
     */
    void run();


    ~EventLoop();

private:
    Epoll* _M_ep;
};