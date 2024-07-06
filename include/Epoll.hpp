#pragma once

#include <iostream>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <strings.h>
#include <sys/epoll.h>
#include <vector>
#include <unistd.h>

#include "Channel.hpp"

class Channel;

/**
 *  封住了epoll的三种主要方法和其所需的变量
 */
class Epoll
{
public:

    /**
     *  
     * @describe: 调用epoll_create()初始化_M_epoll_fd
     * @param:    void
     * 
     */
    Epoll();


    /**
     *  
     * @describe: 监听Channel或者修改Channel的监视事件, 封装了epoll_ctl方法
     * @param:    Channel*
     * @return:   void
     * 
     */
    void updata_channel(Channel* ch_ptr);


    /**
     * 
     * @describe: 从红黑树上删除Channel
     * @param:    Channel*
     * @return:   void
     */
    void remove(Channel* ch_ptr);


    /**
     * 
     * @describe: 调用epoll_wait(), 返回所有响应的fd对应的Channel* 
     * @param:    int
     * @return:   std::vector<Channel*>
     */
    std::vector<Channel*> wait(int time_out = -1);

    ~Epoll();
    
private:
    static const int _M_max_events = 100;
    int _M_epoll_fd = -1;
    epoll_event _M_events_arr[_M_max_events];
};