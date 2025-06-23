/**
 * 
 * Channel头文件
 * 
 */
#ifndef CHANNEL_H
#define CHANNEL_H

#include <functional>
#include <memory>
#include <sys/epoll.h>

#include "base/TimeStamp.h"
#include "base/noncopyable.h"

namespace mymuduo {
namespace net {

class Socket;
class EventLoop;

enum channelStatus {
    kNew = -1,          // channel从未被注册过
    kAdded = 1,         // channel已注册, 已监听
    kDeleted = 2        // channel已注册, 未监听
};

/**
 * 封装了sockfd感兴趣的event, 还绑定了poller返回的具体事节
 */
class Channel : noncopyable
{
public:

    using EventCallbacl = std::function<void()>;
    using ReadCallback = std::function<void(TimeStamp)>;

public:

    /**
     * @brief 将fd与事件循环绑定
     */
    Channel(EventLoop* loop_ptr, int fd);

    /**
     * @brief 事件发生后的处理函数, 会回调事件的函数
     */
    void handle(TimeStamp receiveTime);

    /**
     * @brief 四种事件
     */
    void set_read_events();
    void set_write_events();
    void unset_read_events();
    void unset_write_events();
    void unset_all_events();

    /**
     * @brief 从事件循环中删除Channel
     */
    void remove();

    /**
     * @brief 设置或者取消边缘触发
     */
    void set_ET();
    void unset_ET();

    /**
     * @brief 设置, 获取事件
     */
    void set_happened_events(uint32_t events);
    uint32_t get_happened_events();
    uint32_t get_monitored_events();

    /**
     * @brief 设置回调函数
     */
    void set_read_callback(ReadCallback cb);   // 读事件
    void set_write_callback(EventCallbacl cb); // 写事件
    void set_close_callback(EventCallbacl cb); // 关闭连接
    void set_error_callback(EventCallbacl cb); // 错误事件

    bool is_none_events() { return _M_monitored_events == _M_none_events; }
    bool is_reading() { return _M_monitored_events == _M_read_events; }
    bool is_writing() { return _M_monitored_events == _M_write_events; }

    /**
     * @brief 将channel与obj绑定在一起, 在TcpConnection建立时绑定
     */
    void tie(const std::shared_ptr<void>& obj);

    channelStatus  get_status() { return _M_status; }
    void set_status(channelStatus status) { _M_status = status; }

    int get_fd();
    EventLoop* owner_loop() { return _M_loop_ptr; }

private:

    void update();
    void handle_event_with_guard(TimeStamp receiveTime);

    static const int _M_read_events = EPOLLIN | EPOLLPRI;
    static const int _M_write_events = EPOLLOUT;
    static const int _M_none_events = 0;

    const int _M_fd = -1;               // poller监听的对象
    EventLoop* _M_loop_ptr = nullptr;   // 事件循环
    
    bool _M_in_epoll = false;
    uint32_t _M_monitored_events = 0;
    uint32_t _M_happened_events = 0;
    channelStatus _M_status;           // channel在Poller中的状态(未添加, 已添加, 已删除)

    /**
     * 用于保存channel和其绑定对象的弱引用, 绑定对象通常是TcpConnection
     *  1. 避免循环引用
     *  2. 延长对象声明周期: 确保在channel的回调期间, TcpConnection对象不会被销毁
     */
    std::weak_ptr<void> _M_tie; // 调用成员方法lock()可以将对象提升为强引用
    bool _M_tied;

    // 读事件的回调函数, 将回调Acceptor::new_connection或者new_message
    ReadCallback _M_read_callback; 

    // 写事件的回调函数, 将回调Connection::send
    EventCallbacl _M_write_callback;

    // 连接关闭的回调函数, 将回调Connection::close_callback
    EventCallbacl _M_close_callback; 

    // 连接出错的回调函数, 将回调Connection::error_callback
    EventCallbacl _M_error_callback; 
};

} // namespace net
} // namespace mymuduo

#endif // CHANNEL_H