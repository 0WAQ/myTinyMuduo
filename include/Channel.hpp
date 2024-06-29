#pragma once
#include <sys/epoll.h>
#include "Epoll.hpp"

class Epoll;

class Channel
{
public:

    /**
     * 
     * @describe: 初始化该Channel对应的fd与epoll_fd
     * @prama:    epfd -> Epoll*
     *            fd   -> int
     * 
     */
    Channel(Epoll* epfd, int fd);


    /**
     * 
     * @describe: 获取该Channel实例对应的fd
     * @param:    void
     * @return:   int
     */
    int get_fd();


    /**
     * 
     * @describe: 设置该fd为边缘触发
     * @param:    void
     * @return:   void
     * 
     */
    void set_ET();


    /**
     * 
     * @describe: 监听该fd的读事件
     * @prama:    void
     * @return:   void
     * 
     */
    void set_read_events();


    /**
     * 
     * @describe: 设置该Channel对应的fd已经被监听
     * @prama:    void
     * @return:   void
     * 
     */
    void set_in_epoll();


    /**
     * 
     * @describe: 判断该Channel对应的fd是否被监听
     * @prama:    void
     * @return:   bool
     * 
     */
    bool get_in_epoll();


    /**
     * 
     * @describe: 设置该fd发生的事件, 在epoll_wait()之后
     * @prama:    uint32_t
     * @return:   void
     * 
     */
    void set_happened_events(uint32_t events);


    /**
     * 
     * @describe: 获取该fd发生的事件
     * @prama:    void
     * @return:   uint32_t
     * 
     */
    uint32_t get_happened_events();


    /**
     * 
     * @describe: 获取fd被epoll监听的事件
     * @prama:    void
     * @return:   uint32_t
     * 
     */
    uint32_t get_monitored_events();


    ~Channel();

private:
    int _M_fd = -1;
    Epoll* _M_ep = nullptr;
    bool _M_in_epoll = false;
    uint32_t _M_monitored_events = 0;
    uint32_t _M_happened_events = 0;
};