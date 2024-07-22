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
#include "Logger.hpp"

class Channel;


/// @brief 封装Epoll
class Epoll
{
public:

    /// @brief 调用epoll_create, 初始化epfd
    Epoll();


    /// @brief 调用epoll_ctl, 修改ch监听事件
    /// @param ch 
    void updata_channel(Channel* ch);


    /// @brief 取消监听ch的事件
    /// @param ch 
    void remove_channel(Channel* ch);


    /// @brief 调用epoll_wait, 返回发生事件的合集
    /// @param time_out 超时时间
    /// @return 发生的事件合集
    std::vector<Channel*> wait(int time_out = -1);


    ~Epoll();
    
private:

    // 发生事件的最大数量
    static const int _M_max_events = 100;

    // epfd
    int _M_epoll_fd = -1;

    // 发生事件的合集
    epoll_event _M_events_arr[_M_max_events];
};