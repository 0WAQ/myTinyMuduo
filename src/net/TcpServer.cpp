#include "base/Logger.h"
#include "net/callbacks.h"
#include "net/TcpServer.h"
#include "net/TcpConnection.h"

#include <memory>
#include <mutex>

using namespace mymuduo;
using namespace mymuduo::net;

namespace mymuduo::net {
namespace __detail {

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
} // namespace mymuduo::net

TcpServer::TcpServer(EventLoop *main_loop, const InetAddress &serv_addr,
                     const std::string &name, Option option, bool is_ET) :
        _M_main_loop(__detail::check_loop_not_null(main_loop)),
        _M_ip_port(serv_addr.ip_port()),
        _M_name(name),
        _M_acceptor(new Acceptor(main_loop, serv_addr, option == kReusePort)),
        _M_loop_threads(new EventLoopThreadPool(main_loop, name)),
        _M_next(1), _M_started(0), _M_stopping(false), _M_is_ET(is_ET)
{
    _M_acceptor->set_new_connection_callback(std::bind(&TcpServer::new_connection, this,
                std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer() {
    if (!_M_stopping) {
        LOG_WARN("TcpServer is destroyed without calling stop().");
        stop();
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

void TcpServer::stop() {
    if (_M_stopping) {
        return;
    }

    _M_stopping.store(true);


    _M_main_loop->run_in_loop([this] {
        _M_acceptor->stop();
    });


    for(auto& item : _M_connections) {
        // MARK: 用临时的智能指针获取 TcpConnection 对象
        std::shared_ptr<TcpConnection> conn { item.second };
        item.second.reset();

        // 关闭连接
        conn->loop()->run_in_loop(std::bind(&TcpConnection::force_close, conn));
    }

    // 等待所有连接关闭
    std::unique_lock<std::mutex> lock { _M_connections_mutex };
    while (_M_connections.size() > 0) {
        _M_connections_cond.wait_for(lock, 300ms);
    }

    _M_loop_threads->stop();
}

void TcpServer::new_connection(int clntfd, const InetAddress &clnt_addr)
{
    assert(_M_main_loop->is_loop_thread());

    // 填充TcpConnection名称
    char buf[64] = {0};
    size_t id = _M_next++;

    snprintf(buf, sizeof(buf), "-%s#%lu", _M_ip_port.c_str(), id);
    std::string connName = _M_name + buf;
    
    LOG_INFO("TcpServer::new_connection [%s] - new connection [%s] from %s.\n",
        _M_name.c_str(), connName.c_str(), clnt_addr.ip_port().c_str());

    InetAddress local_addr(InetAddress::get_local_addr(clntfd));

    // 分配TcpConnection给相应的loop
    EventLoop *nextLoop = _M_loop_threads->get_next_loop();

    // MARK: 将TcpConnection用shared_ptr管理
    //      1. TcpConnection直接与用户交互, 无法相信用户!!!
    //      2. TcpConnection是临界资源, 为防止在一个线程使用该对象时被其它连接释放
    std::shared_ptr<TcpConnection> conn { new TcpConnection(nextLoop
                                                    , id, connName
                                                    , clntfd, local_addr
                                                    , clnt_addr, _M_is_ET) };

    _M_connections[id] = conn;    // 用哈希表管理连接

    // 设置回调函数
    conn->set_connection_callback(_M_connection_callback);
    conn->set_message_callback(_M_message_callback);
    conn->set_write_complete_callback(_M_write_complete_callback);
    conn->set_close_callback(std::bind(&TcpServer::remove_connection, this, std::placeholders::_1));

    // 让对应的loop建立连接
    nextLoop->run_in_loop(std::bind(&TcpConnection::established, conn));
}

void TcpServer::remove_connection(const TcpConnectionPtr &conn)
{
    // MARK: 该函数是连接断开后 从Reactor线程 执行的回调
    //       但TcpConnection对象是 主Reactor线程 创建的, 主Reactor线程 要在其哈希表中删除该对象
    
    // DONE: 这里修改为由主线程去删除
    _M_main_loop->run_in_loop(std::bind(&TcpServer::remove_connection_in_loop, this, conn));
}

void TcpServer::remove_connection_in_loop(const TcpConnectionPtr &conn)
{
    LOG_INFO("TcpServer::remove_connection_in_loop [%s] - connection %s.\n",
                _M_name.c_str(), conn->name().c_str());
    
    // MARK: 在 主Reactor线程 中删除该对象
    size_t id = conn->id();
    _M_connections.erase(id);

    {
        std::lock_guard<std::mutex> guard { _M_connections_mutex };
        if (_M_connections.size() == 0) {
            _M_connections_cond.notify_one();
        }
    }

    // MARK: 然后让TcpConnection对象所属的 从Reactor线程 去销毁连接
    conn->loop()->run_in_loop(std::bind(&TcpConnection::destroyed, conn));
}

