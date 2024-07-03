#include "../include/Connection.hpp"

Connection::Connection(EventLoop* loop, Socket* clnt_sock) 
                    : _M_loop_ptr(loop), _M_clnt_sock_ptr(clnt_sock)
{
    _M_clnt_channel_ptr = new Channel(_M_loop_ptr, _M_clnt_sock_ptr->get_fd());
    
    // 设置clnt_channel的执行函数为new_message
    _M_clnt_channel_ptr->set_read_callback(std::bind(&Connection::new_message, this));
    // 设置Channel需要回调的函数
    _M_clnt_channel_ptr->set_close_callback(std::bind(&Connection::close_callback, this));
    _M_clnt_channel_ptr->set_error_callback(std::bind(&Connection::error_callback, this));

    // 设置为边缘触发
    _M_clnt_channel_ptr->set_ET();
    // 设置为读事件, 并监听
    _M_clnt_channel_ptr->set_read_events();
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

void Connection::close_callback()
{
    _M_close_callback(this);
}

void Connection::error_callback()
{
    _M_error_callback(this);
}

void Connection::set_close_callback(std::function<void(Connection*)> func)  {
    _M_close_callback = func;
}

void Connection::set_error_callback(std::function<void(Connection*)> func)  {
    _M_error_callback = func;
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
            printf("recv(clnt_fd = %d): %s\n", get_fd(), _M_input_buffer.data());
            
            /**
             * 将数据经过计算后
             */

            _M_output_buffer = _M_input_buffer; // echo
            _M_input_buffer.clear();
            send(get_fd(), _M_output_buffer.data(), _M_output_buffer.size(), 0);

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

Connection::~Connection()
{
    delete _M_clnt_sock_ptr;
    delete _M_clnt_channel_ptr;
}