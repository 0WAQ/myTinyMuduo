#include "../include/Connection.hpp"

Connection::Connection(EventLoop* loop, std::unique_ptr<Socket> clnt_sock) 
        : _M_loop_ptr(loop), _M_clnt_sock_ptr(std::move(clnt_sock)), 
          _M_clnt_channel_ptr(new Channel(_M_loop_ptr, _M_clnt_sock_ptr->get_fd())), 
          _M_is_diconnected(false)
{
    
    // 设置Connection的回调函数
    _M_clnt_channel_ptr->set_read_callback(std::bind(&Connection::new_message, this));
    _M_clnt_channel_ptr->set_write_callback(std::bind(&Connection::write_events, this));
    _M_clnt_channel_ptr->set_close_callback(std::bind(&Connection::close_callback, this));
    _M_clnt_channel_ptr->set_error_callback(std::bind(&Connection::error_callback, this));

    // 设置为边缘触发
    _M_clnt_channel_ptr->set_ET();
    // 监听读事件
    _M_clnt_channel_ptr->set_read_events();
}

void Connection::write_events()
{
    // 当可写后, 尝试把用户缓冲区的数据全部发送出去
    int len = ::send(get_fd(), _M_output_buffer.data(), _M_output_buffer.size(), 0);
    // 因为操作系统的原因(tcp滑动窗口), 数据不一定能全部接收, 剩下的数据等待下一次写事件触发

    // 将成功发送的数据从用户缓冲区中删除掉
    if(len > 0) {
        _M_output_buffer.erase(0, len);
    }
    
    // 若缓冲区中没有数据了, 表示数据已成功发送, 不再关注写事件
    if(_M_output_buffer.size() == 0) {
        _M_clnt_channel_ptr->unset_write_events();
        _M_send_complete_callback(shared_from_this());
    }
}

// 封装消息发送
void Connection::send(const char* data, size_t size)
{  
    // 若连接已经断开
    if(_M_is_diconnected) {
        std::cout << "连接提前断开, send()直接返回." << std::endl;
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
        _M_loop_ptr->push(std::bind(&Connection::send_a, this, message));
    }
}

void Connection::send_a(std::shared_ptr<std::string> message)
{
    // 先将数据发送到用户缓冲区中
    _M_output_buffer.append_with_sep(message->data(), message->size());

    // 注册写事件, 用来判断内核缓冲区是否可写. 若可写, Channel会回调write_events函数
    _M_clnt_channel_ptr->set_write_events();
}

void Connection::new_message()
{
    char buf[1024];

    // 只用边缘触发, 需要确保将发送过来的数据读取完毕
    while(true) // 非阻塞IO
    {
        // 初始化buf为0
        bzero(&buf, sizeof(buf));

        ssize_t nlen = read(get_fd(), buf, sizeof(buf));

        // 数据读取成功
        if(nlen > 0) 
        {
            _M_input_buffer.append(buf, nlen); // 将数据添加到用户缓冲区中
        }
        // 读取时被中断
        else if(nlen == -1 && errno == EINTR) 
        {
            continue;
        }
        // 非阻塞读的行为, 全部的数据已读取完毕(即目前的Socket缓冲区中没有数据)
        else if(nlen == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) 
        {
            // 每次读完后, 将数据以报文为单位循环发送出去
            while(true)
            {
                // 取出一个报文
                std::string message;
                if(!_M_input_buffer.pick_datagram(message))
                    break;

                // 更新时间戳
                _M_ts = TimeStamp();

                _M_deal_message_callback(shared_from_this(), message);
            }

            break;
        }
        // 连接断开
        else if(nlen == 0) 
        {
            close_callback();
            break;
        }
    }
}

bool Connection::timer_out(time_t val) {
    return time(0) - _M_ts.to_time_t() > val;
}

void Connection::close_callback()
{
    _M_is_diconnected = true;
    // 从事件循环中删除Channel
    _M_clnt_channel_ptr->remove();
    // 调用回调函数, 转交给TcpServer处理
    _M_close_callback(shared_from_this());
}

void Connection::error_callback()
{
    _M_is_diconnected = true;
    // 从事件循环中删除Channel
    _M_clnt_channel_ptr->remove();
    // 调用回调函数, 转交给TcpServer处理
    _M_error_callback(shared_from_this());
}

int Connection::get_fd() {
    return _M_clnt_sock_ptr->get_fd();
}

std::string Connection::get_ip() const {
    return _M_clnt_sock_ptr->get_ip();
}

uint16_t Connection::get_port() const {
    return _M_clnt_sock_ptr->get_port();
}

void Connection::set_close_callback(std::function<void(Connection_ptr)> func)  {
    _M_close_callback = func;
}

void Connection::set_error_callback(std::function<void(Connection_ptr)> func)  {
    _M_error_callback = func;
}

void Connection::set_send_complete_callback(std::function<void(Connection_ptr)> func) {
    _M_send_complete_callback = func;
}

void Connection::set_deal_message_callback(std::function<void(Connection_ptr, std::string&)> func) {
    _M_deal_message_callback = func;
}

Connection::~Connection() {

}