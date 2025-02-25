#include "Connection.h"

Connection::Connection(EventLoop* loop, std::unique_ptr<Socket> sock, bool is_ET) 
        : _M_loop_ptr(loop), _M_sock_ptr(std::move(sock)), _M_is_ET(is_ET),
          _M_channel_ptr(new Channel(_M_loop_ptr, _M_sock_ptr->get_fd())), 
          _M_is_diconnected(false)
{
    
    // 设置Connection被channel回调的四种函数
    _M_channel_ptr->set_read_callback(std::bind(&Connection::read_events, this));
    _M_channel_ptr->set_write_callback(std::bind(&Connection::write_events, this));
    _M_channel_ptr->set_close_callback(std::bind(&Connection::close_events, this));
    _M_channel_ptr->set_error_callback(std::bind(&Connection::error_events, this));

    LOG_INFO("Connection is created[fd = %d].\n", this->get_fd());

    // 设置为边缘触发
    if(_M_is_ET) {
        _M_channel_ptr->set_ET();
    }

    // 监听读事件
    _M_channel_ptr->set_read_events();
}

/// 四种事件函数, 被channel调用

void Connection::read_events()
{
    char buf[1024];

    // 若边缘触发, 需要确保将发送过来的数据读取完毕, TODO: ET => LT
    while(true) // 非阻塞IO
    {
        int save_error = 0;

        // 将数据直接读取到输入缓冲区
        ssize_t nlen = _M_input_buffer.read_fd(get_fd(), &save_error);

        // 数据读取成功
        if(nlen > 0) 
        {
            LOG_DEBUG("Connection::read_events[fd=%d], read %ld bytes to input_buffer\n", 
                    this->get_fd(), nlen);
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
                    LOG_WARN("Connection::read_eventes[fd=%d], can't pick a datagram\n", 
                            this->get_fd());
                    break;
                }

                // 更新时间戳
                _M_ts = TimeStamp();

                _M_deal_message_callback(shared_from_this(), message);
            }

            break;
        }
        // 连接断开
        else if(nlen == 0) 
        {
            close_events();
            break;
        }
    }
}

void Connection::write_events()
{
    int save_error = 0;
    // 当可写后, 尝试把用户缓冲区的数据全部发送出去
    ssize_t nlen = _M_output_buffer.write_fd(get_fd(), &save_error);
    // 因为操作系统的原因(tcp滑动窗口), 数据不一定能全部接收, 剩下的数据等待下一次写事件触发

    if(nlen < 0) {
        LOG_ERROR("%s:%s:%d write_fd error:%d.\n", 
            __FILE__, __FUNCTION__, __LINE__, save_error);
    }
    else // 数据发送成功
    {
        // 若发送后payload为0, 表示数据全部发送, 不再关注写事件
        if(_M_output_buffer.readable() == 0) {
            _M_channel_ptr->unset_write_events();
            _M_send_complete_callback(shared_from_this());
        }
    }
}

// 在两个地方被调用: 1.channel的handle中; 2.channel回调的read_events中
void Connection::close_events()
{
    _M_is_diconnected = true;
    // 从事件循环中删除Channel
    _M_channel_ptr->remove();
    // 调用回调函数, 转交给TcpServer处理
    _M_close_callback(shared_from_this());
}

void Connection::error_events()
{
    _M_is_diconnected = true;
    // 从事件循环中删除Channel
    _M_channel_ptr->remove();
    // 调用回调函数, 转交给TcpServer处理
    _M_error_callback(shared_from_this());
}


// 判断最后一次接收到数据的时间到现在, 有没有超过val
bool Connection::is_expired(time_t val) { return time(0) - _M_ts.to_time_t() > val; }



// 封装消息发送, 选择由IO线程执行
void Connection::send(const char* data, size_t size)
{  
    // 若连接已经断开
    if(_M_is_diconnected) {
        LOG_DEBUG("Connection %p had been disconnected.\n");
        return;
    }

    std::shared_ptr<std::string> message(new std::string(data, size));

    // 判断当前线程是否为IO线程
    if(_M_loop_ptr->is_loop_thread()) // 若是IO线程, 直接执行send_a
    {
        send_a(message);
    }
    else // 若是工作线程, 交由IO线程执行
    {
        // 添加到loop的任务队列中
        _M_loop_ptr->queue_in_loop(std::bind(&Connection::send_a, this, message));
    }
}

// TODO:
void Connection::send_a(std::shared_ptr<std::string> message)
{
    // 将数据发送到用户缓冲区中
    _M_output_buffer.append(*message);

    // 注册写事件, 用来判断内核缓冲区是否可写. 若可写, Channel会回调write_events函数
    _M_channel_ptr->set_write_events();
}



// 读,写,关闭,错误 四个设置回调函数
void Connection::set_deal_message_callback(DealMsgCallback func) {_M_deal_message_callback = std::move(func);}
void Connection::set_send_complete_callback(SendCompleteCallback func) {_M_send_complete_callback = std::move(func);}
void Connection::set_close_callback(CloseCallback func) {_M_close_callback = std::move(func);}
void Connection::set_error_callback(ErrorCallback func) {_M_error_callback = std::move(func);}



// 获取fd, ip, port
int Connection::get_fd() const { return _M_sock_ptr->get_fd();}
std::string Connection::get_ip() const { return _M_sock_ptr->get_ip();}
uint16_t Connection::get_port() const { return _M_sock_ptr->get_port();}
