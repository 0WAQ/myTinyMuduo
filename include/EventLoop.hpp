#pragma once
#include <memory>
#include <functional>
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


    /**
     * 
     * @describe: 转调用Epoll::remove
     * @param:    Channel*
     * @return:   void
     */
    void remove(Channel* ch_ptr);


    /**
     * 
     * @describe: 设置回调函数
     * @param:    void
     * @return:   void
     */
    void set_epoll_timeout_callback(std::function<void(EventLoop*)> func);


    ~EventLoop();

private:

    std::unique_ptr<Epoll> _M_ep_ptr;

    // 当epoll_wait()超时时, 回调TcpServer::epoll_timeout()
    std::function<void(EventLoop*)> _M_epoll_wait_timeout_callback;
};