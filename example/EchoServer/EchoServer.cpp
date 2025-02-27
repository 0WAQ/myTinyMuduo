/**
 * 
 * EchoServer实现cpp文件
 * 
 */

#include "EchoServer.h"
#include "InetAddress.h"

EchoServer::EchoServer(EventLoop *main_loop, InetAddress addr, size_t loop_thread_num, size_t work_thread_num) 
        : _M_tcp_server(main_loop, addr, ""), _M_thread_num(work_thread_num), 
          _M_pool("WORK", _M_thread_num)
{

    // 业务需要什么事件,就回调什么事件
    // _M_tcp_server.set_deal_message_callback(std::bind(&EchoServer::handle_deal_message_a, this, 
    //                                         std::placeholders::_1, std::placeholders::_2));

    // _M_tcp_server.set_close_connection_callback(std::bind(&EchoServer::handle_close_connection, this,
    //                                         std::placeholders::_1));

    // _M_tcp_server.set_error_connection_callback(std::bind(&EchoServer::handle_error_connection, this,
    //                                         std::placeholders::_1));

    // _M_tcp_server.set_create_connection_callback(std::bind(&EchoServer::handle_create_connection, this,
    //                                         std::placeholders::_1));

    // _M_tcp_server.set_send_complete_callback(std::bind(&EchoServer::handle_send_complete, this,
    //                                         std::placeholders::_1));

    // _M_tcp_server.set_timer_out_callback(std::bind(&EchoServer::handle_timer_out, this, 
    //                                         std::placeholders::_1));
}

void EchoServer::start() 
{
    LOG_INFO("EchoServer启动中...\n");
    _M_tcp_server.set_thread_num(2);
    _M_tcp_server.start();
}

void EchoServer::stop() 
{
    _M_is_stop = true;

    _M_pool.stop();

    // TODO: 暂停TcpServer
}

void EchoServer::handle_deal_message_a(TcpConnectionPtr conn, std::string& message)
{   
    // 若没有工作线程, 再调用回去, 由IO线程处理
    if(_M_pool.size() == 0) 
    {
        handle_deal_message(conn, message);
    }
    else
    {
        // 将业务添加到线程池的工作队列中a
        _M_pool.push(std::bind(&EchoServer::handle_deal_message, this, conn, message));
    }

}

void EchoServer::handle_deal_message(TcpConnectionPtr conn, std::string& message)
{
    // 将数据经过计算后             
    message = "reply: " + message;

    // 将报文发送出去
    conn->send(message);
}

void EchoServer::handle_create_connection(TcpConnectionPtr conn)
{
    printf("%s: new connection(ip=%s, fd=%d).\n", 
                TimeStamp::now().to_string().c_str(), 
                conn->peer_address().get_ip().c_str(), conn->get_fd());
}

void EchoServer::handle_close_connection(TcpConnectionPtr conn) 
{
    printf("%s: close connection(ip=%s, fd=%d).\n", 
                TimeStamp::now().to_string().c_str(), 
                conn->peer_address().get_ip().c_str(), conn->get_fd());
}

void EchoServer::handle_error_connection(TcpConnectionPtr conn) 
{
    printf("%s: error connection(ip=%s, fd=%d).\n", 
                TimeStamp::now().to_string().c_str(),
                conn->peer_address().get_ip().c_str(), conn->get_fd());
}

void EchoServer::handle_send_complete(TcpConnectionPtr conn) 
{
    // printf("%s: send complete(ip=%s, fd=%d).\n", 
    //             TimeStamp::now().to_string().c_str(),
    //             conn->peer_address().get_ip().c_str(), conn->get_fd());
}

void EchoServer::handle_timer_out(TcpConnectionPtr conn)
{
    printf("%s: timer_out(ip=%s, fd=%d).\n", 
                TimeStamp::now().to_string().c_str(),
                conn->peer_address().get_ip().c_str(), conn->get_fd());
}

EchoServer::~EchoServer() 
{
    if(!_M_is_stop)
        stop();
}
