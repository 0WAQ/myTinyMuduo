#include "../include/Connection.hpp"

Connection::Connection(EventLoop* loop, Socket* clnt_sock) 
                    : _M_loop_ptr(loop), _M_clnt_sock_ptr(clnt_sock)
{
    _M_clnt_channel_ptr = new Channel(_M_loop_ptr, _M_clnt_sock_ptr->get_fd());
    
    // 设置clnt_channel的执行函数为new_message
    _M_clnt_channel_ptr->set_read_callback(std::bind(&Channel::new_message, _M_clnt_channel_ptr));
    // 设置Channel需要回调的函数
    _M_clnt_channel_ptr->set_close_callback(std::bind(&Connection::close_connection, this));
    _M_clnt_channel_ptr->set_error_callback(std::bind(&Connection::error_connection, this));
    
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

void Connection::close_connection() 
{
    printf("client(clnt_fd = %d) disconnected\n", get_fd());
    close(get_fd());
}

void Connection::error_connection() 
{
    printf("client(clnt_fd = %d) error.\n", get_fd());
    close(get_fd());
}

Connection::~Connection()
{
    delete _M_clnt_sock_ptr;
    delete _M_clnt_channel_ptr;
}