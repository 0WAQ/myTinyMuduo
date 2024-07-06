#include "../include/TcpServer.hpp"


TcpServer::TcpServer(const std::string& ip, const uint16_t port, size_t thread_num) 
                                                : _M_thread_num(thread_num)
{
    _M_main_loop = new EventLoop;
    // _M_main_loop->set_epoll_timeout_callback(std::bind(&TcpServer::epoll_timeout, this, std::placeholders::_1));

    // 主循环运行Acceptor
    _M_acceptor_ptr = new Acceptor(_M_main_loop, ip, port);
    _M_acceptor_ptr->set_create_connection_callback(
        std::bind(&TcpServer::create_connection, this, std::placeholders::_1));

    // // 线程池运行Connection
    _M_pool_ptr = new ThreadPool("IO", _M_thread_num);

    // 创建从事件循环
    for(int i = 0; i < _M_thread_num; i++) 
    {
        _M_sub_loops.push_back(new EventLoop); // 将从事件放入sub_loops中
        // 设置超时回调函数
        _M_sub_loops[i]->set_epoll_timeout_callback(std::bind(&TcpServer::epoll_timeout, this, std::placeholders::_1));
        // 将EventLoop的run函数作为任务添加给线程池
        _M_pool_ptr->push(std::bind(&EventLoop::run, _M_sub_loops[i]));
    }
}

void TcpServer::start()
{
    _M_main_loop->run();
}

void TcpServer::deal_message(Connection_ptr conn, std::string& message)
{
    if(_M_deal_message_callback)
        _M_deal_message_callback(conn, message);
}

void TcpServer::create_connection(Socket* clnt_sock)
{
    // 创建Connection对象, 并将其指定给线程池中的loop  
    Connection_ptr conn(new Connection(_M_sub_loops[clnt_sock->get_fd() % _M_thread_num], clnt_sock));
    conn->set_close_callback(std::bind(&TcpServer::close_connection, this, std::placeholders::_1));
    conn->set_error_callback(std::bind(&TcpServer::error_connection, this, std::placeholders::_1));
    conn->set_send_complete_callback(std::bind(&TcpServer::send_complete, this, std::placeholders::_1));
    conn->set_deal_message_callback(std::bind(&TcpServer::deal_message, this, 
                                                    std::placeholders::_1, std::placeholders::_2));

    // 将连接用map来管理
    _M_connections_map[conn->get_fd()] = conn;

    if(_M_create_connection_callback)
        _M_create_connection_callback(conn);
}

void TcpServer::close_connection(Connection_ptr conn)
{
    if(_M_close_connection_callback)
        _M_close_connection_callback(conn);

    _M_connections_map.erase(conn->get_fd());
}

void TcpServer::error_connection(Connection_ptr conn)
{
    if(_M_error_connection_callback)
        _M_error_connection_callback(conn);

    _M_connections_map.erase(conn->get_fd());
}

void TcpServer::send_complete(Connection_ptr conn)
{
    if(_M_send_complete_callback)
        _M_send_complete_callback(conn);
}

void TcpServer::epoll_timeout(EventLoop* loop)
{
    if(_M_epoll_timeout_callback)
        _M_epoll_timeout_callback(loop);
}

TcpServer::~TcpServer()
{
    delete _M_acceptor_ptr;
    delete _M_main_loop;

    // 释放从事件循环
    for(auto& i : _M_sub_loops) {
        delete i;
    }

    // 释放线程池
    delete _M_pool_ptr;
}

void TcpServer::set_deal_message_callback(std::function<void(Connection_ptr,std::string &message)> func) {
    _M_deal_message_callback = func;
}

void TcpServer::set_create_connection_callback(std::function<void(Connection_ptr)> func) {
    _M_create_connection_callback = func;
}

void TcpServer::set_close_connection_callback(std::function<void(Connection_ptr)> func) {
    _M_close_connection_callback = func;
}

void TcpServer::set_error_connection_callback(std::function<void(Connection_ptr)> func) {
    _M_error_connection_callback = func;
}

void TcpServer::set_send_complete_callback(std::function<void(Connection_ptr)> func) {
    _M_send_complete_callback = func;
}

void TcpServer::set_epoll_timeout_callback(std::function<void(EventLoop*)> func) {
    _M_epoll_timeout_callback = func;
}
