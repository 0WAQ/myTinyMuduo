#include <mymuduo/net/TcpServer.h>
#include <mymuduo/base/Logger.h>

#include <print>

using namespace mymuduo;
using namespace mymuduo::net;
using namespace std::placeholders;

class EchoServer {
public:
    EchoServer(const InetAddress &servAddr)
            : _loop(std::make_unique<EventLoop>())
            , _server(std::make_unique<TcpServer>(_loop.get(), servAddr, "EchoServer"))
    {
        _server->set_connection_callback(std::bind(&EchoServer::onConnection, this, _1));
        _server->set_message_callback(std::bind(&EchoServer::onMessage, this, _1, _2, _3));
        _server->set_thread_num(3);
    }

    void start() {
        _server->start();
    }

    EventLoop* loop() const noexcept {
        return _loop.get();
    }

private:
    void onConnection(const TcpConnectionPtr &conn) {
        if(conn->connected()) {
            std::println("conn UP: {}", conn->peer_address().ip_port());
        }
        else {
            std::println("conn DOWN: {}", conn->peer_address().ip_port());
        }
    }

    void onMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp time) {
        std::string msg;
        if(buf->pick_datagram(msg)) {
            add_sep(buf, msg);
            conn->send(msg);
        }
        else {
            std::println("pick_datagram() return false.");
        }
    }

    void add_sep(Buffer *buf, std::string &message) {
        switch (buf->sep()) {
        case 0:
            break;
        
        case 1: {
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

private:
    std::unique_ptr<EventLoop> _loop;
    std::unique_ptr<TcpServer> _server;
};