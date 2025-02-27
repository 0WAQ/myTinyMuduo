#include "TcpServer.h"

namespace __detail
{

    /**
     * @brief 判断loop非空, 确保用户不会传入空的main_loop
     */
    static EventLoop* check_loop_not_null(EventLoop *loop) {
        if(!loop) {
            LOG_ERROR("%s:%s:%d main loop is null!", __FILE__, __FUNCTION__, __LINE__);
        }
        return loop;
    }

} // namespace __detail

TcpServer::TcpServer(EventLoop *main_loop, InetAddress &serv_addr,
                     const std::string &name, Option option) :
        _M_main_loop(__detail::check_loop_not_null(main_loop)),
        _M_ip_port(serv_addr.get_ip_port()),
        _M_name(name),
        _M_acceptor(new Acceptor(main_loop, serv_addr, option == kReusePort)),
        _M_loop_threads(new EventLoopThreadPool(main_loop, name)),
        _M_connection_callback(),
        _M_message_callback(),
        _M_next(1), _M_started(0)
{
    _M_acceptor->set_new_connection_callback(std::bind(&TcpServer::new_connection, this,
                std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer() { }

// 启动服务器
void TcpServer::start() 
{
    LOG_INFO("TcpServer启动中...\n");

    // 防止被多次启动
    if(_M_started++ == 0) {

        // 从事件循环
        _M_loop_threads->start(_M_thread_init_callback);
        
        // 主事件循环
        _M_main_loop->run_in_loop(std::bind(&Acceptor::listen, _M_acceptor.get()));
    }
}


// 以下是被调函数
void TcpServer::new_connection(int clntfd, const InetAddress &clnt_addr)
{
    EventLoop *curr = _M_loop_threads->get_next_loop();
    char buf[64] = {0};
    snprintf(buf, sizeof(buf), "-%s#%d", _M_ip_port.c_str(), ++_M_next);
    std::string connName = _M_name + buf;

    LOG_INFO("TcpServer::new_connection [%s] - new connection [%s] from %s.\n",
                _M_name.c_str(), connName.c_str(), clnt_addr.get_ip_port().c_str());
    
    InetAddress local_addr;
    
    
    TcpConnectionPtr conn(new TcpConnection(curr, connName, clntfd, local_addr, clnt_addr));
    _M_connection_map[connName] = conn;
        
    // conn->set_close_callback(std::bind(&TcpServer::close_connection, this, std::placeholders::_1));
    // conn->set_error_callback(std::bind(&TcpServer::error_connection, this, std::placeholders::_1));
    // conn->set_send_complete_callback(std::bind(&TcpServer::send_complete, this, std::placeholders::_1));
    // conn->set_deal_message_callback(std::bind(&TcpServer::deal_message, this, 
    //                                                 std::placeholders::_1, std::placeholders::_2));

    ///////////////////////////////////////////////////////////////
    // {
    //     std::lock_guard<std::mutex> lock(_M_mutex);

    //     // 将conn存放到TcpServer的map容器中, 用于在连接中断时释放连接
    //     _M_connection_map[conn->get_fd()] = conn;
    // }
    ///////////////////////////////////////////////////////////////

    // curr->run_in_loop(std::bind(&TcpConnection::))

    // // 将conn存放到EventLoop的map容器中, 用于处理定时器事件
    // _M_sub_loops[fd % _M_IO_thread_num]->insert(conn);

    // if(_M_connection_callback) {
    //     _M_connection_callback(conn);
    // }
    
    LOG_DEBUG("TcpServer::create_connection[fd=%d]\n", conn->get_fd());
}
