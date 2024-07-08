#include "../include/TcpServer.hpp"


TcpServer::TcpServer(const std::string& ip, const uint16_t port, size_t thread_num) 
    : _M_thread_num(thread_num), _M_main_loop(new EventLoop(true)), 
      _M_acceptor(_M_main_loop.get(), ip, port), _M_pool("IO", _M_thread_num)
{
    
    // _M_main_loop->set_epoll_timeout_callback(std::bind(&TcpServer::epoll_timeout, this, std::placeholders::_1));
    
    _M_acceptor.set_create_connection_callback(
        std::bind(&TcpServer::create_connection, this, std::placeholders::_1));

    // 创建从事件循环
    for(int i = 0; i < _M_thread_num; i++) 
    {
        _M_sub_loops.emplace_back(new EventLoop(false, 5, 10)); // 将从事件放入sub_loops中
        // 设置epoll_wait超时回调函数
        _M_sub_loops[i]->set_epoll_timeout_callback(std::bind(&TcpServer::epoll_timeout, this, std::placeholders::_1));
        // 设置定时器超时回调函数, 用于清理空闲超时Connection
        _M_sub_loops[i]->set_timer_out_callback(std::bind(&TcpServer::remove_conn, this, std::placeholders::_1));
        // 将EventLoop的run函数作为任务添加给线程池
        _M_pool.push(std::bind(&EventLoop::run, _M_sub_loops[i].get()));
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

void TcpServer::create_connection(std::unique_ptr<Socket> clnt_sock)
{
    // 先获取fd, 因为在调用Connection构造时, 从右到左入栈, 已经将clnt_sock的资源转移出去了
    int fd = clnt_sock->get_fd();
    // 创建Connection对象, 并将其指定给线程池中的loop  
    Connection_ptr conn(new Connection(
        _M_sub_loops[fd % _M_thread_num].get(), std::move(clnt_sock)));
    
    conn->set_close_callback(std::bind(&TcpServer::close_connection, this, std::placeholders::_1));
    conn->set_error_callback(std::bind(&TcpServer::error_connection, this, std::placeholders::_1));
    conn->set_send_complete_callback(std::bind(&TcpServer::send_complete, this, std::placeholders::_1));
    conn->set_deal_message_callback(std::bind(&TcpServer::deal_message, this, 
                                                    std::placeholders::_1, std::placeholders::_2));

    ///////////////////////////////////////////////////////////////
    {
        std::lock_guard<std::mutex> lock(_M_mutex);

        // 将conn存放到TcpServer的map容器中, 用于在连接中断时释放连接
        _M_connections_map[conn->get_fd()] = conn;
    }
    ///////////////////////////////////////////////////////////////

    // 将conn存放到EventLoop的map容器中, 用于处理定时器事件
    _M_sub_loops[fd % _M_thread_num]->push(conn);

    if(_M_create_connection_callback)
        _M_create_connection_callback(conn);
}

void TcpServer::close_connection(Connection_ptr conn)
{
    if(_M_close_connection_callback)
        _M_close_connection_callback(conn);

    ///////////////////////////////////////////////////////////////
    {
        std::lock_guard<std::mutex> lock(_M_mutex);

        _M_connections_map.erase(conn->get_fd());
    }
    ///////////////////////////////////////////////////////////////
    
}

void TcpServer::error_connection(Connection_ptr conn)
{
    if(_M_error_connection_callback)
        _M_error_connection_callback(conn);

    ///////////////////////////////////////////////////////////////
    {
        std::lock_guard<std::mutex> lock(_M_mutex);

        _M_connections_map.erase(conn->get_fd());
    }
    ///////////////////////////////////////////////////////////////
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

// 清理空闲的Connection
void TcpServer::remove_conn(int fd)
{
    ///////////////////////////////////////////////////////////////
    {
        std::lock_guard<std::mutex> lock(_M_mutex);

        _M_connections_map.erase(fd);
    }
    ///////////////////////////////////////////////////////////////
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

TcpServer::~TcpServer() {

}