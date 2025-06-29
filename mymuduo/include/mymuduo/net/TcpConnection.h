/**
 * 
 * Connection头文件
 * 
 */
#ifndef CONNECTION_H
#define CONNECTION_H

#include <unistd.h>
#include <sys/syscall.h>
#include <memory>
#include <atomic>

#include "mymuduo/base/TimeStamp.h"
#include "mymuduo/base/noncopyable.h"
#include "mymuduo/net/callbacks.h"
#include "mymuduo/net/Socket.h"
#include "mymuduo/net/InetAddress.h"
#include "mymuduo/net/Channel.h"
#include "mymuduo/net/Buffer.h"


namespace mymuduo {
namespace net {

class EventLoop;
class Channel;
class TcpConnection;


/**
 *  Channel之上的封装类, 专门用于创建客户端的Socket
 *  继承自这个类, 用shared_from_this代替this指针
 */
class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection>
{
public:

    friend class TcpConnectionAccessor;

    using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

    enum State {
        kConnecting,        // 连接建立中 (默认状态)
        kConnected,         // 连接已建立 (活跃状态)
        kDisConnecting,     // 连接断开中
        kDisConnected       // 连接已断开 (最终状态)
    };

public:


    /**
     * @brief 将sock绑定到事件循环
     * @param loop 从事件循环
     */
    TcpConnection(EventLoop *loop, size_t id, const std::string &name, int clntfd,
                const InetAddress &localAddr, const InetAddress &clntAddr, bool is_ET = false);

    ~TcpConnection();

    /**
     * @brief 将send交由IO线程执行
     */
    void send(const std::string &message);

    /**
     * @brief 关闭连接 (写端)
     */
    void shutdown();
    
    /**
     * @brief 强制关闭连接
     */
    void force_close();
    void force_close_with_delay(TimeDuration delay);

    /**
     * @brief 建立连接
     */
    void established();
    
    /**
     * @brief 销毁连接, 用于在连接关闭后清理对象资源
     */
    void destroyed();

    /**
     * @brief 用户可以设置的回调函数
     */
    void set_connection_callback(ConnectionCallback func) { _M_connection_callback = std::move(func); }
    void set_message_callback(MessageCallback func) { _M_message_callback = std::move(func); }
    void set_write_complete_callback(WriteCompleteCallback func) { _M_write_complete_callback = std::move(func); }
    void set_close_callback(CloseCallback func) { _M_close_callback = std::move(func); }
    void set_high_water_mark_callback(HighWaterMarkCallback func) { _M_high_water_mark_callback = std::move(func); }
    
    void set_high_water_mark(size_t high_water_mark) { _M_high_water_mark = high_water_mark; }
    const size_t high_water_mark() const { return _M_high_water_mark; }
    int fd() const { return _M_sock->fd(); }
    size_t id() const { return _M_id; }
    const std::string& name() const { return _M_name; }
    const InetAddress& local_address() { return _M_local_addr; }
    const InetAddress& peer_address() { return _M_peer_addr; }
    EventLoop* loop() const { return _M_loop; }
    const int state() const { return _M_state.load(); }
    bool connected() const { return _M_state == kConnected; }

private:

    /**
     * @brief 四种事件
     */
    void handle_read_ET(TimeStamp receieveTime);
    void handle_read_LT(TimeStamp receieveTime);
    void handle_write();
    void handle_close();
    void handle_error();


    void send_in_loop(const void* data, size_t len);
    void shutdown_in_loop();
    void force_close_in_loop();

private:

        std::atomic<int> _M_state;
        bool _M_reading;
        size_t _M_high_water_mark;      // 水位标志

        // 从事件循环
        EventLoop* _M_loop;

        std::string _M_name;

        const size_t _M_id;      // 连接的编号

    /**
     * 
     */
        // Socket及其信息
        std::unique_ptr<Socket> _M_sock;
        std::unique_ptr<Channel> _M_channel;

        InetAddress _M_local_addr;
        InetAddress _M_peer_addr;

    /**
     * 用户缓冲区
     */

        Buffer _M_input_buffer;
        Buffer _M_output_buffer;

    /**
     * 回调函数
     */

        ConnectionCallback _M_connection_callback;
        MessageCallback _M_message_callback;
        WriteCompleteCallback _M_write_complete_callback;
        CloseCallback _M_close_callback;
        HighWaterMarkCallback _M_high_water_mark_callback;
};

} // namespace net
} // namespace mymuduo

#endif // CONNECTION_H