#include "mymuduo/base/Logger.h"
#include "mymuduo/net/TcpConnection.h"
#include "mymuduo/net/EventLoop.h"

#include <cassert>
#include <cerrno>

using namespace mymuduo;
using namespace mymuduo::net;

namespace mymuduo::net {
namespace __detail {

    /**
     * @brief 判断loop非空, 确保用户不会传入空的main_loop
     */
    static EventLoop* check_loop_not_null(EventLoop *loop) {
        if(!loop) {
            LOG_ERROR("{}:{}:{} sub loop is null!",
                __FILE__, __FUNCTION__, __LINE__);
        }
        return loop;
    }

} // namespace __detail

void default_connection_callback(const TcpConnectionPtr& conn) {
    LOG_DEBUG("{} -> {} is {}", conn->local_address().ip_port(),
                    conn->peer_address().ip_port(),
                    (conn->connected()) ? "UP" : "DOWN");
}

void default_message_callback(const TcpConnectionPtr& conn, Buffer* buf, Timestamp t) {
    buf->retrieve_all();
}

} // namespace mymuduo::net

TcpConnection::TcpConnection(EventLoop *loop, size_t id, const std::string &name, int clntfd, 
                        const InetAddress &localAddr, const InetAddress &clntAddr, bool is_ET) :
            _loop(__detail::check_loop_not_null(loop)),
            _id(id),
            _name(name),
            _state(kConnecting),
            _reading(true),
            _sock(new Socket(clntfd)),
            _channel(new Channel(loop, clntfd)),
            _local_addr(localAddr),
            _peer_addr(clntAddr),
            _input_buffer(),
            _output_buffer(),
            _high_water_mark(64*1024*1024)
{
    // 设置Connection被channel回调的四种函数
    _channel->set_write_callback(std::bind(&TcpConnection::handle_write, this));
    _channel->set_close_callback(std::bind(&TcpConnection::handle_close, this));
    _channel->set_error_callback(std::bind(&TcpConnection::handle_error, this));
    if(is_ET) {
        _channel->set_read_callback(std::bind(&TcpConnection::handle_read_ET, this, std::placeholders::_1));
        _channel->set_ET();
    }
    else {
        _channel->set_read_callback(std::bind(&TcpConnection::handle_read_LT, this, std::placeholders::_1));
    }
    
    // 保活机制
    _sock->set_keep_alive(true);

    // MARK: 默认不监听写事件, 否则在LT模式下, 只要内核缓冲区可读, 就会一直触发谢事件!!!
    //       只有当用户缓冲区中有数据时才会监听写事件

    LOG_INFO("TcpConnection::ctor[{}] at fd={} in thread#{}.", _name, clntfd, CurrentThread::tid());
}

TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection::dtor[{}] at fd={} in thread#{}.", _name, _channel->fd(), CurrentThread::tid());
}

void TcpConnection::established()
{
    assert(_loop->is_loop_thread());
    assert(_state == kConnecting);
   
    
    // MARK: 如何保证TcpConnection对象在执行回调时不会被意外销毁???
    //       在建立时将当前TcpConnection对象绑定到channel的tie中
    //       tie是一个weak_ptr, 正常情况不会增加TcpConnection对象的引用
    //       但是在执行channel的handle时, 会将该weak_ptr提升为shared_ptr(临时增加引用计数
    //       就不会在执行毁掉的过程中被销毁了
    
    _state = kConnected;
    _channel->tie(shared_from_this()); // 将该Connection与Channel绑定
    _channel->set_read_events();

    LOG_INFO("TcpConnection::established[{}] at fd={} in thread#{}.", _name, _channel->fd(), CurrentThread::tid());
    
    if(_connection_callback) {
        _connection_callback(shared_from_this());
    }
}

void TcpConnection::destroyed()
{
    // MARK: 每个类成员函数内都有一个隐式的this指针(若继承自enable_shared_from_this则为shared_ptr)

    assert(_loop->is_loop_thread());
    LOG_INFO("TcpConnection::destroyed[{}] at fd={} in thread#{}.", _name, _channel->fd(), CurrentThread::tid());

    if(_state == kConnected)
    {
        _state = kDisConnected;

        // 连接关闭后, 就不能监听事件了
        _channel->unset_all_events();
        
        if (_connection_callback) {
            _connection_callback(shared_from_this());
        }
    }
    _channel->remove();    
}

void TcpConnection::handle_read_ET(Timestamp receieveTime)
{
    assert(_loop->is_loop_thread());

    while(true) // 因为是ET模式, 所以要确保数据一次性读完
    {
        int save_error = 0;

        // 将数据直接读取到输入缓冲区
        ssize_t nlen = _input_buffer.read_fd(_channel->fd(), &save_error);

        // 数据读取成功
        if(nlen > 0) 
        {
            LOG_DEBUG("TcpConnection::handle_read[fd={}], read {} bytes to input_buffer", 
                        _channel->fd(), nlen);
            continue;
        }
        // 读取时被中断
        else if(nlen == -1 && errno == EINTR) 
        {
            LOG_WARN("{}:{}:{} read_fd:{} was interrupted.", 
                __FILE__, __FUNCTION__, __LINE__, save_error);
            continue;
        }
        // 非阻塞读在没有读到数据时会返回 -1, 表示全部的数据已读取完毕(即目前的Socket缓冲区中没有数据)
        else if(nlen == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) 
        {
            _message_callback(shared_from_this(), &_input_buffer, receieveTime);
            break;
        }
        // 连接断开
        else if(nlen == 0) 
        {
            handle_close();
            break;
        }
    }
}

void TcpConnection::handle_read_LT(Timestamp receieveTime)
{
    assert(_loop->is_loop_thread());

    int save_error = 0;
    ssize_t nlen = _input_buffer.read_fd(_channel->fd(), &save_error);

    if(nlen > 0) {
        // MARK: 还要将接受到数据的缓冲区也交给上层服务器
        LOG_INFO("TcpConnection::handle_read[{}] at fd={} in thread#{}.", _name, _channel->fd(), CurrentThread::tid());
        _message_callback(shared_from_this(), &_input_buffer, receieveTime);
    }
    else if(nlen == 0) {
        LOG_INFO("TcpConnection::handle_close[{}] at fd={} in thread#{}.", _name, _channel->fd(), CurrentThread::tid());
        handle_close();
    }
    else {
        errno = save_error;
        LOG_ERROR("TcpConnection::handle_error[{}] at fd={} in thread#{}.", _name, _channel->fd(), CurrentThread::tid());
        handle_error();
    }
}

void TcpConnection::handle_write()
{
    assert(_loop->is_loop_thread());

    if(_channel->is_writing())
    {
        int save_error = 0;
        // 当可写后, 尝试把用户缓冲区的数据全部发送出去
        ssize_t nlen = _output_buffer.write_fd(fd(), &save_error);
        // 因为操作系统的原因(tcp滑动窗口), 数据不一定能全部接收, 剩下的数据等待下一次写事件触发

        if(nlen < 0) {
            LOG_WARN("{}:{}:{} TcpConnection::handle_write() error:{}.", 
                __FILE__, __FUNCTION__, __LINE__, save_error);
        }
        else // 数据发送成功
        {
            // 若发送后payload为0, 表示数据全部发送, 不再关注写事件
            if(_output_buffer.readable() == 0) {

                // MARK: 将可写事件关闭掉, 防止在LT模式下内核一直触发而导致影响性能
                _channel->unset_write_events();

                // 调用写完回调
                if(_write_complete_callback) {
                    _loop->run_in_loop(std::bind(_write_complete_callback, shared_from_this()));
                }
            }

            if(_state == kDisConnecting) {
                shutdown_in_loop();
            }
        }
    }
    else
    {
        LOG_WARN("TcpConnection fd={} is down, no more writin.", _channel->fd());
    }
}

// 在两个地方被调用: 1.channel的handle中; 2.channel回调的read_events中
void TcpConnection::handle_close()
{
    assert(_loop->is_loop_thread());
    assert(_state == kConnected || _state == kDisConnecting);

    LOG_INFO("fd={} state={}.", _channel->fd(), (int)_state);

    _state = kDisConnected;

    // 从事件循环中删除Channel
    _channel->unset_all_events();

    TcpConnectionPtr conn(shared_from_this());
    if(_connection_callback) {
        _connection_callback(conn);
    }

    // 调用回调函数, 转交给TcpServer处理
    if(_close_callback) {
        _close_callback(conn);
    }
}

void TcpConnection::handle_error()
{
    _state = kDisConnected;

    // 从事件循环中删除Channel
    _channel->unset_all_events();
    _channel->remove();
}

// 封装消息发送, 选择由IO线程执行
void TcpConnection::send(const std::string& message)
{
    if(_state == kConnected)
    {
        // 判断当前线程是否为IO线程
        if(_loop->is_loop_thread()) // 若是IO线程, 直接执行send_a
        {
            send_in_loop(message.data(), message.size());
        }
        else // 若是工作线程, 交由IO线程执行
        {
            // 添加到loop的任务队列中
            _loop->run_in_loop(std::bind(&TcpConnection::send_in_loop, this, message.data(), message.size()));
        }
    }
    else
    {
        LOG_DEBUG("TcpConnection {} had been disconnected or connecting.", fd());
    }

}

void TcpConnection::send_in_loop(const void *data, size_t len)
{
    assert(_loop->is_loop_thread());
    
    if(_state == kDisConnected)
    {
        LOG_WARN("disconnected, give up writing.");
        return;
    }
    
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool fault_error = false;

    // 第一次发送数据, 或者缓冲区没有待发送数据
    if(!_channel->is_writing() && _output_buffer.readable() == 0)
    {
        // 先将数据直接写入fd
        nwrote = ::write(_channel->fd(), data, len);

        if(nwrote > 0) {
            remaining = len - nwrote;
            
            // 数据全部发送完成, 就不用再设置可写事件了
            if(remaining == 0 && _write_complete_callback) {
                _loop->queue_in_loop(std::bind(_write_complete_callback, shared_from_this()));
            }
        }
        else // nwrote < 0
        {
            nwrote = 0;
            if(errno != EWOULDBLOCK)
            {
                LOG_WARN("TcpConnection::send_in_loop write to {} failed.", fd());
                if(errno == EPIPE || errno == ECONNRESET) {
                    fault_error = true;
                }
            }
        }
    }

    assert(remaining <= len);

    // 将剩余数据保存到输出缓冲区中
    if(!fault_error && remaining > 0)
    {
        // 发送缓冲区中剩余的待发送数据的长度
        size_t oldLen = _output_buffer.readable();

        // 原本没有超过水位, 这次超过水位
        if(oldLen + remaining >= _high_water_mark
            && oldLen < _high_water_mark
            && _high_water_mark_callback)
        {
            _loop->run_in_loop(std::bind(_high_water_mark_callback, shared_from_this(), oldLen + remaining));
        }

        _output_buffer.append_with_sep(std::string((char*)data, len));

        // MARK: 若channel没有关注可写事件, 则关注
        if(!_channel->is_writing()) {
            _channel->set_write_events();
        }
    }
}

void TcpConnection::shutdown()
{
    if(_state == kConnected)
    {
        _state = kDisConnecting;
        _loop->run_in_loop(std::bind(&TcpConnection::shutdown_in_loop, this));
    }
}

void TcpConnection::shutdown_in_loop()
{
    // 说明output_buffer没有数据
    if(!_channel->is_writing())
    {
        _sock->shutdown_write();  // 关闭写端, 会触发EPOLLHUP, 会触发close_callback        
    }
}

void TcpConnection::force_close()
{
    if (_state == kConnected || _state == kDisConnecting) {
        _state = kDisConnecting;
        _loop->queue_in_loop(std::bind(&TcpConnection::force_close_in_loop, shared_from_this()));
    }
}

void TcpConnection::force_close_with_delay(TimeDuration delay) {
    if (_state == kConnected || _state == kDisConnecting) {
        _state = kDisConnecting;

        // 使用 weak_ptr, 防止对象的生命周期被定时器延长
        auto weak_conn = weak_from_this();
        _loop->run_after(delay, [weak_conn] {
            if (auto conn = weak_conn.lock()) {
                conn->force_close();
            }
        });
    }
}

void TcpConnection::force_close_in_loop()
{
    assert(_loop->is_loop_thread());
    if (_state == kConnected || _state == kDisConnecting) {
        handle_close();
    }
}

