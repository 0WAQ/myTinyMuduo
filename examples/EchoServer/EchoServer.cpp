#include "EchoServer.h"

int main(int argc, char* argv[]) {
    if(argc != 3) {
        std::println("Usage: ./server <ip> <port>>");
        std::println("Example: ./server 127.0.0.1 5678");
        return -1;
    }

    Logger::instance().set_log_level(Logger::WARN);
    Logger::instance().set_output([](const char* data, size_t len) {
        std::fprintf(stdout, data, len);
    });

    InetAddress addr(argv[1], atoi(argv[2]));
    auto server = std::make_unique<EchoServer>(addr);
    server->start();

    server->loop()->run_every(5s, [](){
        std::println("run every 5s.");
    });

    server->loop()->loop();

    return 0;
}