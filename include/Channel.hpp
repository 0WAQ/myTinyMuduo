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
    Channel(EventLoop* loop, int fd);


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
     * @param:    void
     * @return:   void
     * 
     */
    void set_read_events();


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
     * @describe: 封装处理新的连接请求的代码
     * @param:    服务端sock
     * @return:   void
     * 
     */
    void new_connection(Socket* serv_sock);


    /**
     * 
     * @describe: 封装处理新消息的代码
     * @param:    void
     * @return:   void
     * 
     */
    void new_message();


    /**
     * 
     * @describe: 设置回调函数
     * @param:    待执行的回调函数
     * @return:   void
     * 
     */
    void set_read_callback(std::function<void()> func);


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
    EventLoop* _M_loop = nullptr;
    bool _M_in_epoll = false;
    uint32_t _M_monitored_events = 0;
    uint32_t _M_happened_events = 0;
    std::function<void()> _M_read_callback; // 读事件的回调函数
};