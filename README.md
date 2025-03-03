# myTinyMuduo: 一个多线程C++网络库

## 简介
该项目是仿照muduo实现的一个基于Reactor模式的多线程C++网络库，使用C++11对项目进行了重构。
- 多线程依赖于C++11提供的`thread`库, 而不是重新封装`POSIX thread API`.
- 时间戳依赖于C++11提供的`chrono`库, 而不是直接使用`gettimeofday()`.
- 仅使用`Epoll`作为事件分发器.
## 示例
一个简单的`Echo`服务器如下:
```c++
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
```
这个实现非常简单, 用户只需包含`TcpServer.h`头文件, 它将包含其余文件. 在实现时, 用户只需关注事件的回调即可. 在`Buffer`类的内部使用了两种解决`TCP`粘包问题的方法.

`main`函数如下:
```c++
int main(int argc, char* argv[])
{
    if(argc != 4) {
        std::cout << "Usage: ./tcp_epoll <ip> <port> <log_dir>\n";
        std::cout << "Example: ./tcp_epoll 127.0.0.1 5678 ../../log\n";
        return -1;
    }

    mymuduo::Logger* log = mymuduo::Logger::get_instance();
    log->init(DEBUG, argv[3], ".log");

    mymuduo::EventLoop loop;
    mymuduo::InetAddress addr(argv[1], atoi(argv[2]));
    std::string name{"EchoServer-01"};

    EchoServer *server = new EchoServer(&loop, addr, name);
    server->start();

    loop.run_every(1.0, [](){
        LOG_INFO("run every 1s.\n");
    });

    loop.loop();

    return 0;
}
```
## 安装
```shell
$ git clone git@github.com:0WAQ/myTinyMuduo.git
$ cd myTinyMuduo
$ ./build.sh
```
## 参考资料
    1. 陈硕的muduo库: https://github.com/chenshuo/muduo
    2. TinyWebServer: https://github.com/qinguoyi/TinyWebServer
    3. 博客: https://www.cnblogs.com/S1mpleBug/p/16712003.html
    4. 书籍:《Linux多线程服务端编程: 使用muduo C++网络库》