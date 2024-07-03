#pragma once
#include <sys/epoll.h>
#include <functional>

#include "EventLoop.hpp"
#include "Socket.hpp"

class Socket;
class EventLoop;

/**
 * 
 * 包含了一个EventLoop*的成员变量(封装了一个Epoll对象)
 * 封装了让Epoll对象监视Socket对象所需的各种变量
 */
class Channel
{
public:

    /**
     * 
     * @describe: 初始化该Channel对应的fd与epoll_fd
     * @param:    epfd -> Epoll*
     *            fd   -> int
     * 
     */
    Channel(EventLoop* loop_ptr, int fd);


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
     * @describe: 注册读事件
     * @param:    void
     * @return:   void
     * 
     */
    void set_read_events();


    /**
     * 
     * @describe: 注册写事件
     * @param:    void
     * @return:   void
     */
    void set_write_events();
    
    
    /**
     * 
     * @describe: 取消读事件
     * @param:    void
     * @return:   void
     */
    void unset_read_events();


    /**
     * 
     * @describe: 取消写事件
     * @param:    void
     * @return:   void
     */
    void unset_write_events();


    /**
     * 
     * @describe: 设置该Channel对应的fd已经被监听
     * @param:    void
     * @return:   void
     * 
     */
    void set_in_epoll();


    /**
     * 
     * @describe: 判断该Channel对应的fd是否被监听
     * @param:    void
     * @return:   bool
     * 
     */
    bool get_in_epoll();


    /**
     * 
     * @describe: 设置该fd发生的事件, 在epoll_wait()之后
     * @param:    uint32_t
     * @return:   void
     * 
     */
    void set_happened_events(uint32_t events);


    /**
     * 
     * @describe: 获取该fd发生的事件
     * @param:    void
     * @return:   uint32_t
     * 
     */
    uint32_t get_happened_events();


    /**
     * 
     * @describe: 获取fd被epoll监听的事件
     * @param:    void
     * @return:   uint32_t
     * 
     */
    uint32_t get_monitored_events();


    /**
     * 
     * @describe: 设置回调函数
     * @param:    待执行的回调函数
     * @return:   void
     * 
     */
    void set_read_callback(std::function<void()> func);  // 读事件
    void set_close_callback(std::function<void()> func); // 关闭连接
    void set_error_callback(std::function<void()> func); // 错误事件


    /**
     * 
     * @describe: 封装epoll_wait()之后的处理逻辑
     * @param:    void
     * @return:   void
     */
    void handle();

    ~Channel();

private:
    int _M_fd = -1;
    EventLoop* _M_loop_ptr = nullptr;
    bool _M_in_epoll = false;
    uint32_t _M_monitored_events = 0;
    uint32_t _M_happened_events = 0;

    // 读事件的回调函数, 将回调Acceptor::new_connection或者new_message
    std::function<void()> _M_read_callback; 

    // 连接关闭的回调函数, 将回调Connection::close_callback
    std::function<void()> _M_close_callback; 

    // 连接出错的回调函数, 将回调Connection::error_callback
    std::function<void()> _M_error_callback; 
};