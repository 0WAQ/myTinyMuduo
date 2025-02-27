#include "TcpConnection.h"
#include "Logger.h"
#include <errno.h>

namespace __detail
{

    /**
     * @brief 判断loop非空, 确保用户不会传入空的main_loop
     */
    static EventLoop* check_loop_not_null(EventLoop *loop) {
        if(!loop) {
            LOG_ERROR("%s:%s:%d sub loop is null!", __FILE__, __FUNCTION__, __LINE__);
        }
        return loop;
    }

} // namespace __detail

TcpConnection::TcpConnection(EventLoop *loop, const std::string &name, int clntfd, 
                        const InetAddress &localAddr, const InetAddress &clntAddr, bool _is_ET) :
            _M_loop(__detail::check_loop_not_null(loop)),
            _M_name(name),
            _M_state(kConnecting),
            _M_reading(true),
            _M_is_ET(false),
            _M_sock(new Socket(clntfd)),
            _M_channel(new Channel(loop, clntfd)),
            _M_local_addr(localAddr),
            _M_peer_addr(clntAddr),
            _M_high_water_mark(64*1024*1024)
{
    // 设置Connection被channel回调的四种函数
    _M_channel->set_read_callback(std::bind(&TcpConnection::handle_read, this, std::placeholders::_1));
    _M_channel->set_write_callback(std::bind(&TcpConnection::handle_write, this));
    _M_channel->set_close_callback(std::bind(&TcpConnection::handle_close, this));
    _M_channel->set_error_callback(std::bind(&TcpConnection::handle_error, this));

    // 设置为边缘触发
    if(_M_is_ET) {
        _M_channel->set_ET();
    }

    // 保护机制
    _M_sock->set_keep_alive(true);

    // 监听读事件
    _M_channel->set_read_events();

    LOG_INFO("TcpConnection::ctor[%s] at fd=%d.\n", _M_name.c_str(), clntfd);
}

TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection::dtor[%s] at fd=%d.\n", _M_name.c_str(), _M_channel->get_fd());
}

void TcpConnection::established()
{
    _M_state = kConnected;

    _M_channel->tie(shared_from_this());
    _M_channel->set_read_events();

    if(_M_connection_callback) {
        _M_connection_callback(shared_from_this());
    }
}

void TcpConnection::destroyed()
{
    if(_M_state == kConnected)
    {
        _M_state = kDisConnected;
    
        _M_channel->unset_all_events();
    
        if(_M_close_callback) {
            _M_close_callback(shared_from_this());
        }
    }
    _M_channel->remove();
}

void TcpConnection::handle_read(TimeStamp receieveTime)
{
    // 若边缘触发, 需要确保将发送过来的数据读取完毕, TODO: ET => LT
    if(_M_is_ET) {
        handle_read_ET(receieveTime);
    }
    else {
        handle_read_LT(receieveTime);
    }

}

void TcpConnection::handle_read_ET(TimeStamp receieveTime)
{
    char buf[1024] = {0};

    while(true) // 非阻塞IO
    {
        int save_error = 0;

        // 将数据直接读取到输入缓冲区
        ssize_t nlen = _M_input_buffer.read_fd(_M_channel->get_fd(), &save_error);

        // 数据读取成功
        if(nlen > 0) 
        {
            LOG_DEBUG("TcpConnection::handle_read[fd=%d], read %ld bytes to input_buffer\n", 
                        _M_channel->get_fd(), nlen);
            continue;
        }
        // 读取时被中断
        else if(nlen == -1 && errno == EINTR) 
        {
            LOG_WARN("%s:%s:%d read_fd:%d was interrupted.\n", 
                __FILE__, __FUNCTION__, __LINE__, save_error);
            continue;
        }
        // 非阻塞读的行为, 全部的数据已读取完毕(即目前的Socket缓冲区中没有数据)
        else if(nlen == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) 
        {
            // 读取完成后, 
            while(true)
            {
                // 取出一个报文
                std::string message;
                if(!_M_input_buffer.pick_datagram(message)) {
                    LOG_WARN("TcpConnection::read_eventes[fd=%d], can't pick a datagram\n", 
                            _M_channel->get_fd());
                    break;
                }

                // TODO:
                _M_message_callback(shared_from_this(), &_M_input_buffer, receieveTime);
            }

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

void TcpConnection::handle_read_LT(TimeStamp receieveTime)
{
    int save_error = 0;
    ssize_t nlen = _M_input_buffer.read_fd(_M_channel->get_fd(), &save_error);

    if(nlen > 0) {
        _M_message_callback(shared_from_this(), &_M_input_buffer, receieveTime);
    }
    else if(nlen == 0) {
        handle_close();
    }
    else {
        errno = save_error;
        LOG_ERROR("TcpConnection::handle_read_LT.\n");
        
        handle_error();
    }
}

void TcpConnection::handle_write()
{
    if(_M_channel->is_writing())
    {
        int save_error = 0;
        // 当可写后, 尝试把用户缓冲区的数据全部发送出去
        ssize_t nlen = _M_output_buffer.write_fd(get_fd(), &save_error);
        // 因为操作系统的原因(tcp滑动窗口), 数据不一定能全部接收, 剩下的数据等待下一次写事件触发

        if(nlen < 0) {
            LOG_WARN("%s:%s:%d TcpConnection::handle_write() error:%d.\n", 
                __FILE__, __FUNCTION__, __LINE__, save_error);
        }
        else // 数据发送成功
        {
            // 若发送后payload为0, 表示数据全部发送, 不再关注写事件
            if(_M_output_buffer.readable() == 0) {
                _M_channel->unset_write_events();
                if(_M_write_complete_callback) {
                    _M_loop->run_in_loop(std::bind(_M_write_complete_callback, shared_from_this()));
                }
            }

            if(_M_state == kDisConnecting) {
                shutdown_in_loop();
            }
        }
    }
    else
    {
        LOG_WARN("TcpConnection fd=%d is down, no more writin.\n", _M_channel->get_fd());
    }
}

// 在两个地方被调用: 1.channel的handle中; 2.channel回调的read_events中
void TcpConnection::handle_close()
{
    LOG_INFO("fd=%d state=%d.\n", _M_channel->get_fd(), (int)_M_state);

    _M_state = kDisConnected;

    // 从事件循环中删除Channel
    _M_channel->remove();

    TcpConnectionPtr conn(shared_from_this());
    if(_M_connection_callback) {
        _M_connection_callback(conn);
    }

    // 调用回调函数, 转交给TcpServer处理
    if(_M_close_callback) {
        _M_close_callback(conn);
    }
}

void TcpConnection::handle_error()
{
    _M_state = kDisConnected;

    // 从事件循环中删除Channel
    _M_channel->remove();
}

// 封装消息发送, 选择由IO线程执行
void TcpConnection::send(const std::string& message)
{  
    if(_M_state == kConnected)
    {
        // 判断当前线程是否为IO线程
        if(_M_loop->is_loop_thread()) // 若是IO线程, 直接执行send_a
        {
            send_in_loop(message.data(), message.size());
        }
        else // 若是工作线程, 交由IO线程执行
        {
            // 添加到loop的任务队列中
            _M_loop->run_in_loop(std::bind(&TcpConnection::send_in_loop, this, message.data(), message.size()));
        }
    }
    else
    {
        LOG_DEBUG("TcpConnection %p had been disconnected.\n");
    }

}

// TODO:
void TcpConnection::send_in_loop(const void *data, size_t len)
{
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool fault_error = false;

    if(_M_state == kDisConnected)
    {
        LOG_WARN("disconnected, give up writing.\n");
        return;
    }

    // 第一次发送数据, 或者缓冲区没有待发送数据
    if(!_M_channel->is_writing() && _M_output_buffer.readable() == 0)
    {
        nwrote = ::write(_M_channel->get_fd(), data, len);
        if(nwrote > 0) {
            remaining = len - nwrote;
            
            // 数据全部发送完成, 就不用再设置可写事件了
            if(remaining == 0 && _M_write_complete_callback) {
                _M_loop->queue_in_loop(std::bind(_M_write_complete_callback, shared_from_this()));
            }
        }
        else // nwrote < 0
        {
            nwrote = 0;
            if(errno != EWOULDBLOCK)
            {
                LOG_ERROR("TcpConnection::send_in_loop");
                if(errno == EPIPE || errno == ECONNRESET) {
                    fault_error = true;
                }
            }
        }
    }

    // 数据没有全部发送出去, 剩余数据需保存在缓冲区中
    if(!fault_error && remaining > 0)
    {
        // 发送缓冲区中剩余的待发送数据的长度
        size_t oldLen = _M_output_buffer.readable();
        if(oldLen + remaining >= _M_high_water_mark
            && oldLen < _M_high_water_mark
            && _M_high_water_mark_callback)
        {
            _M_loop->run_in_loop(std::bind(_M_high_water_mark_callback, shared_from_this(), oldLen + remaining));
        }

        _M_output_buffer.append(std::string((char*)data, len));
        if(!_M_channel->is_writing()) {
            _M_channel->set_write_events();
        }
    }
}

void TcpConnection::shutdown()
{
    if(_M_state = kConnected)
    {
        _M_state = kDisConnecting;
        _M_loop->run_in_loop(std::bind(&TcpConnection::shutdown_in_loop, this));
    }    
}

void TcpConnection::shutdown_in_loop()
{
    // 说明output_buffer没有数据
    if(!_M_channel->is_writing())
    {
        _M_sock->shutdown_write();  // 关闭写端, 会触发EPOLLHUP, 会触发close_callback        
    }
}