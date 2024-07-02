#include "../include/Connection.hpp"

Connection::Connection(EventLoop* loop, Socket* clnt_sock) 
                    : _M_loop_ptr(loop), _M_clnt_sock_ptr(clnt_sock)
{
    _M_clnt_channel_ptr = new Channel(_M_loop_ptr, _M_clnt_sock_ptr->get_fd());
    
    // 设置clnt_channel的执行函数为new_message
    _M_clnt_channel_ptr->set_read_callback(std::bind(&Channel::new_message, _M_clnt_channel_ptr));
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

Connection::~Connection()
{
    delete _M_clnt_sock_ptr;
    delete _M_clnt_channel_ptr;
}