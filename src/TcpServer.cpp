#include "TcpServer.h"

namespace mymuduo
{

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

TcpServer::TcpServer(EventLoop *main_loop, const InetAddress &serv_addr,
                     const std::string &name, Option option, bool is_ET) :
        _M_main_loop(__detail::check_loop_not_null(main_loop)),
        _M_ip_port(serv_addr.get_ip_port()),
        _M_name(name),
        _M_acceptor(new Acceptor(main_loop, serv_addr, option == kReusePort)),
        _M_loop_threads(new EventLoopThreadPool(main_loop, name)),
        _M_next(1), _M_started(0), _M_is_ET(is_ET)
{
    _M_acceptor->set_new_connection_callback(std::bind(&TcpServer::new_connection, this,
                std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer()
{
    for(auto& item : _M_connections) {
        TcpConnectionPtr conn(item.second);
        item.second.reset();
        conn->loop()->run_in_loop(std::bind(&TcpConnection::destroyed, conn));
    }
}

// 启动服务器
void TcpServer::start() 
{
    // 防止被多次启动
    if(_M_started++ == 0) {

        // 启动从EventLoop线程
        _M_loop_threads->start(_M_thread_init_callback);
        
        // 启动主EventLoop
        _M_main_loop->run_in_loop(std::bind(&Acceptor::listen, _M_acceptor.get()));
    }
}

void TcpServer::new_connection(int clntfd, const InetAddress &clnt_addr)
{
    // 填充TcpConnection名称
    char buf[64] = {0};
    snprintf(buf, sizeof(buf), "-%s#%d", _M_ip_port.c_str(), ++_M_next);
    std::string connName = _M_name + buf;
    
    LOG_INFO("TcpServer::new_connection [%s] - new connection [%s] from %s.\n",
        _M_name.c_str(), connName.c_str(), clnt_addr.get_ip_port().c_str());

    InetAddress local_addr(InetAddress::get_local_addr(clntfd));

    // 分配TcpConnection给相应的loop
    EventLoop *nextLoop = _M_loop_threads->get_next_loop();
    TcpConnectionPtr conn(new TcpConnection(nextLoop, connName, clntfd, local_addr, clnt_addr, _M_is_ET));
    
    _M_connections[connName] = conn;    // 用哈希表管理连接

    // 设置回调函数
    conn->set_connection_callback(_M_connection_callback);
    conn->set_message_callback(_M_message_callback);
    conn->set_write_complete_callback(_M_write_complete_callback);
    conn->set_close_callback(std::bind(&TcpServer::remove_connection, this, std::placeholders::_1));

    // 让对应的loop建立连接
    nextLoop->queue_in_loop(std::bind(&TcpConnection::established, conn));
}

void TcpServer::remove_connection(const TcpConnectionPtr &conn)
{
    conn->loop()->run_in_loop(std::bind(&TcpServer::remove_connection_in_loop, this, conn));
}

void TcpServer::remove_connection_in_loop(const TcpConnectionPtr &conn)
{
    LOG_INFO("TcpServer::remove_connection_in_loop [%s] - connection %s.\n",
                _M_name.c_str(), conn->name().c_str());
    
    _M_connections.erase(conn->name());
    conn->loop()->run_in_loop(std::bind(&TcpConnection::destroyed, conn));
}

} // namespace mymuduo