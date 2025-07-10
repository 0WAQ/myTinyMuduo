#include <mymuduo/net/TcpClient.h>
#include <mymuduo/base/Logger.h>

#include <iostream>
#include <print>

using namespace mymuduo;
using namespace mymuduo::net;
using namespace std::placeholders;

class EchoClient {
public:
    EchoClient(const InetAddress& servAddr)
        : _loop(std::make_unique<EventLoop>())
        , _client(std::make_unique<TcpClient>(_loop.get(), servAddr, "EchoClient"))
    {
        _client->set_connection_callback(std::bind(&EchoClient::onConnection, this, _1));
        _client->set_message_callback(std::bind(&EchoClient::onMessage, this, _1, _2, _3));
        _client->set_write_complete_callback(std::bind(&EchoClient::onWriteComplete, this, _1));
    }

    void connect() {
        _client->connect();
    }

    EventLoop* getLoop() {
        return _loop.get();
    }

private:
    void onConnection(const TcpConnectionPtr& conn) {
        if(conn->connected()) {
            std::println("conn UP: {}", conn->peer_address().ip_port());
            std::println("Type Q to quit.");
            handle(conn);
        }
        else {
            std::println("conn DOWN: {}", conn->peer_address().ip_port());
        }
    }

    void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp ts) {
        std::println("From EchoServer: {}", buf->retrieve_all_as_string());
        handle(conn);
    }

    void onWriteComplete(const TcpConnectionPtr& conn) {
        // std::println("write complete: {}", conn->peer_address().ip_port());
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

    void handle(const TcpConnectionPtr& conn) {
        std::print("Please Enter: ");
        std::fflush(stdout);

        std::string msg;
        std::cin >> msg;
        if (msg == "q" || msg == "Q") {
            _loop->queue_in_loop([this] {
                _loop->quit();
            });
            return;
        }
        conn->send(msg);
    }

private:
    std::unique_ptr<EventLoop> _loop;
    std::unique_ptr<TcpClient> _client;
};

int main(int argc, char** argv) {
    if(argc != 3) {
        std::printf("Usage: ./client <ip> <port>>\n");
        std::printf("Example: ./client 127.0.0.1 5678\n");
        return -1;
    }

    Logger::set_log_level(Logger::WARN);

    InetAddress servAddr(argv[1], atoi(argv[2]));
    auto client = std::make_unique<EchoClient>(servAddr);

    client->connect();
    client->getLoop()->loop();
}