/**
 * 
 * 简易的EchoServer实现
 * 
 */

#include <TcpServer.h>

/**
 * @brief 业务服务器: Echo
 */
class EchoServer
{
public:

    using TcpConnectionPtr = std::shared_ptr<mymuduo::TcpConnection>;

    /**
     * @brief 初始化EchoServer
     */
    EchoServer(mymuduo::EventLoop *loop, const mymuduo::InetAddress &addr, const std::string &name) :
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
            LOG_INFO("conn UP : %s\n", conn->peer_address().get_ip_port().c_str());
        }
        else
        {
            LOG_INFO("conn DOWN : %s\n", conn->peer_address().get_ip_port().c_str());
        }
    }

    void onMessage(const TcpConnectionPtr &conn, mymuduo::Buffer *buf, mymuduo::TimeStamp time)
    {
        std::string msg;
        if(buf->pick_datagram(msg)) {
            add_sep(buf, msg);
            conn->send(msg);
        }
        else {
            LOG_WARN("pick_datagram() return false.\n");
        }
        // conn->shutdown();
    }

private:

    void add_sep(mymuduo::Buffer *buf, std::string &message)
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

    mymuduo::EventLoop *loop_;
    mymuduo::TcpServer server_;
};

int main(int argc, char* argv[])
{
    std::string ip{"127.0.0.1"}, path{"log/"};
    int port = 5678;

    switch (argc)
    {
    case 1:
        break;
    case 2:
        path = argv[1];
        break;
    case 3:
        port = atoi(argv[1]);
        path = argv[2];
        break;
    case 4:
        ip = argv[1];
        port = atoi(argv[2]);    
        path = argv[3];
        break;
    default:
        std::cerr << "Usage: ./EchoServer [<ip>] [<port>] [<log_dir>]\n";
        std::cerr << "Example: ./EchoServer 127.0.0.1 5678 log/\n";
        return -1;
    }

    mymuduo::Logger* log = mymuduo::Logger::get_instance(path, "EchoServer");
    log->init(mymuduo::Logger::DEBUG);

    mymuduo::EventLoop loop;
    mymuduo::InetAddress addr(ip, port);
    std::string name{"EchoServer-01"};

    EchoServer *server = new EchoServer(&loop, addr, name);
    server->start();

    loop.run_every(5.0, [](){
        LOG_INFO("run every 5s.\n");
    });

    loop.loop();

    return 0;
}
