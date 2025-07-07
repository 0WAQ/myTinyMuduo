#ifndef MYMUDUO_NET_CHANNEL_H
#define MYMUDUO_NET_CHANNEL_H

#include <functional>
#include <memory>
#include <sys/epoll.h>

#include "mymuduo/base/Timestamp.h"
#include "mymuduo/base/noncopyable.h"

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
class Channel : noncopyable {
public:

    using EventCallback = std::function<void()>;
    using ReadCallback = std::function<void(Timestamp)>;

public:
    /**
     * @brief 将fd与事件循环绑定
     */
    Channel(EventLoop* loop_ptr, int fd);

    /**
     * @brief 事件发生后的处理函数, 会回调事件的函数
     */
    void handle(Timestamp receiveTime);

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
    void set_write_callback(EventCallback cb); // 写事件
    void set_close_callback(EventCallback cb); // 关闭连接
    void set_error_callback(EventCallback cb); // 错误事件

    bool is_none_events() { return _monitored_events == _none_events; }
    bool is_reading() { return _monitored_events == _read_events; }
    bool is_writing() { return _monitored_events == _write_events; }

    /**
     * @brief 将channel与obj绑定在一起, 在TcpConnection建立时绑定
     */
    void tie(const std::shared_ptr<void>& obj);

    channelStatus  get_status() { return _status; }
    void set_status(channelStatus status) { _status = status; }

    int fd();
    EventLoop* owner_loop() { return _loop_ptr; }

private:
    void update();
    void handle_event_with_guard(Timestamp receiveTime);

    static const int _read_events = EPOLLIN | EPOLLPRI;
    static const int _write_events = EPOLLOUT;
    static const int _none_events = 0;

    const int _fd = -1;               // poller监听的对象
    EventLoop* _loop_ptr = nullptr;   // 事件循环
    
    bool _in_epoll = false;
    uint32_t _monitored_events = 0;
    uint32_t _happened_events = 0;
    channelStatus _status;           // channel在Poller中的状态(未添加, 已添加, 已删除)

    /**
     * 用于保存channel和其绑定对象的弱引用, 绑定对象通常是TcpConnection
     *  1. 避免循环引用
     *  2. 延长对象声明周期: 确保在channel的回调期间, TcpConnection对象不会被销毁
     */
    std::weak_ptr<void> _tie; // 调用成员方法lock()可以将对象提升为强引用
    bool _tied;

    // 读事件的回调函数, 将回调Acceptor::new_connection或者new_message
    ReadCallback _read_callback; 

    // 写事件的回调函数, 将回调Connection::send
    EventCallback _write_callback;

    // 连接关闭的回调函数, 将回调Connection::close_callback
    EventCallback _close_callback; 

    // 连接出错的回调函数, 将回调Connection::error_callback
    EventCallback _error_callback; 
};

} // namespace net
} // namespace mymuduo

#endif // MYMUDUO_NET_CHANNEL_H