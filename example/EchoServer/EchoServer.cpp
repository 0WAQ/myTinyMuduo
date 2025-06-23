/**
 * 
 * 简易的EchoServer实现
 * 
 */
#include <net/TcpServer.h>
#include <base/Logger.h>

#include <iostream>

using namespace mymuduo;
using namespace mymuduo::net;

/**
 * @brief 业务服务器: Echo
 */
class EchoServer
{
public:

    using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

    /**
     * @brief 初始化EchoServer
     */
    EchoServer(EventLoop *loop, const InetAddress &addr, const std::string &name) :
            loop_(loop), server_(loop, addr, name)
    {
        server_.set_connection_callback(std::bind(&EchoServer::onConnection, this, std::placeholders::_1));
        server_.set_message_callback(std::bind(&EchoServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        
        server_.set_thread_num(3);
    }

    void start() {
        server_.start();
    }

    void onConnection(const TcpConnectionPtr &conn)
    {
        if(conn->connected())
        {
            LOG_INFO("conn UP : %s\n", conn->peer_address().ip_port().c_str());
        }
        else
        {
            LOG_INFO("conn DOWN : %s\n", conn->peer_address().ip_port().c_str());
        }
    }

    void onMessage(const TcpConnectionPtr &conn, Buffer *buf, TimeStamp time)
    {
        std::string msg;
        if(buf->pick_datagram(msg)) {
            add_sep(buf, msg);
            conn->send(msg);
        }
        else {
            LOG_WARN("pick_datagram() return false.\n");
        }
    }

private:

    void add_sep(Buffer *buf, std::string &message)
    {
        switch (buf->sep())
        {
        case 0:
            break;
        
        case 1:
        {
            int len = message.size();

            char tmp[4]{0};
            memcpy(tmp, &len, 4);
            message.insert(0, tmp, 4);
            break;
        }
        case 2:
            message.append("\r\n\r\n");
            break;
        }   
    }

    EventLoop *loop_;
    TcpServer server_;
};

int main(int argc, char* argv[])
{
    if(argc != 4) {
        std::cout << "Usage: ./tcp_epoll <ip> <port> <log_dir>\n";
        std::cout << "Example: ./tcp_epoll 127.0.0.1 5678 ../../log\n";
        return -1;
    }

    Logger* log = Logger::get_instance(argv[3], "EchoServer");
    log->init(Logger::DEBUG);

    EventLoop loop;
    InetAddress addr(argv[1], atoi(argv[2]));
    std::string name{"EchoServer-01"};

    EchoServer *server = new EchoServer(&loop, addr, name);
    server->start();

    loop.run_every(5s, [](){
        LOG_INFO("run every 5s.\n");
    });

    loop.loop();

    return 0;
}