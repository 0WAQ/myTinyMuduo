#pragma once
#include <unistd.h>
#include <syscall.h>
#include <memory>
#include <functional>
#include <queue>
#include <future>
#include <sys/eventfd.h> // 利用eventfd唤醒线程
#include <sys/timerfd.h> // 定时器
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

    /**
     * 
     * @describe: 初始化对象
     * @param:    time_t, time_t
     * 
     */
    EventLoop(bool main_loop, time_t sec = 5, time_t nsec = 0);


    /**
     * 
     * @describe: 判断当前线程是否为事件循环线程
     * @param:    
     * @return:   bool
     * 
     */
    bool is_loop_thread();


    /**
     * 
     * @describe: 用来存放工作线程的任务
     * @param:    std::function<void()>
     * @return:   void
     * 
     */
    void push(std::function<void()> task);


    /**
     * 
     * @describe: 唤醒事件循环
     * @param:    void
     * @return:   void
     * 
     */
    void notify_one();


    /**
     * 
     * @describe: 唤醒线程后执行的函数
     * @param:    void
     * @return:   void
     */
    void handle_eventfd();


    /**
     * 
     * @describe: 用于处理定时器发生后的待调函数
     * @param:    time_t, time_t
     * @return:   void
     * 
     */
    void handle_timerfd(time_t sec, time_t nsec);


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

    pid_t _M_tid;
    std::unique_ptr<Epoll> _M_ep_ptr;

    // IO线程的任务队列, 用来存放工作线程的任务
    std::queue<std::function<void()>> _M_task_queue;
    std::mutex _M_mutex;  // 任务同步队列的互斥锁

    int _M_efd; // 用于唤醒事件循环的eventfd
    std::unique_ptr<Channel> _M_ech; // 用于将eventfd加入到epoll

    int _M_tfd; // 用于清理空闲Connection的timerfd
    std::unique_ptr<Channel> _M_tch; // 用于将timerfd加入到epoll

    bool _M_is_main_loop; // 用于判断当前线程为主线程还是从线程

    // 当epoll_wait()超时时, 回调TcpServer::epoll_timeout()
    std::function<void(EventLoop*)> _M_epoll_wait_timeout_callback;
};