#include "../include/Connection.hpp"

Connection::Connection(EventLoop* loop, Socket* clnt_sock) 
                    : _M_loop_ptr(loop), _M_clnt_sock_ptr(clnt_sock), _M_is_discon(false)
{
    _M_clnt_channel_ptr = new Channel(_M_loop_ptr, _M_clnt_sock_ptr->get_fd());
    
    // 设置clnt_channel的执行函数为new_message
    _M_clnt_channel_ptr->set_read_callback(std::bind(&Connection::new_message, this));
    // 设置clnt_channel的执行函数为write_events
    _M_clnt_channel_ptr->set_write_callback(std::bind(&Connection::write_events, this));
    // 设置Channel需要回调的函数
    _M_clnt_channel_ptr->set_close_callback(std::bind(&Connection::close_callback, this));
    _M_clnt_channel_ptr->set_error_callback(std::bind(&Connection::error_callback, this));

    // 设置为边缘触发
    _M_clnt_channel_ptr->set_ET();
    // 设置为读事件, 并监听
    _M_clnt_channel_ptr->set_read_events();
}

int Connection::get_fd() {
    return _M_clnt_sock_ptr->get_fd();
}

std::string Connection::get_ip() const {
    return _M_clnt_sock_ptr->get_ip();
}

uint16_t Connection::get_port() const {
    return _M_clnt_sock_ptr->get_port();
}

void Connection::close_callback()
{
    _M_is_discon = true;
    // 从事件循环中删除Channel
    _M_clnt_channel_ptr->remove();
    // 调用回调函数, 转交给TcpServer处理
    _M_close_callback(shared_from_this());
}

void Connection::error_callback()
{
    _M_is_discon = true;
    // 从事件循环中删除Channel
    _M_clnt_channel_ptr->remove();
    // 调用回调函数, 转交给TcpServer处理
    _M_error_callback(shared_from_this());
}

void Connection::set_close_callback(std::function<void(Connection_ptr)> func)  {
    _M_close_callback = func;
}

void Connection::set_error_callback(std::function<void(Connection_ptr)> func)  {
    _M_error_callback = func;
}

void Connection::set_send_complete_callback(std::function<void(Connection_ptr)> func) {
    _M_send_complete_callback = func;
}

void Connection::set_deal_message_callback(std::function<void(Connection_ptr, std::string&)> func) {
    _M_deal_message_callback = func;
}

void Connection::new_message()
{
    char buf[1024];

    // 只用边缘触发, 需要确保将发送过来的数据读取完毕
    while(true) // 非阻塞IO
    {
        // 初始化buf为0
        bzero(&buf, sizeof(buf));

        ssize_t nlen = read(get_fd(), buf, sizeof(buf));

        // 数据读取成功
        if(nlen > 0) 
        {
            _M_input_buffer.append(buf, nlen); // 将数据添加到用户缓冲区中
        }
        // 读取时被中断
        else if(nlen == -1 && errno == EINTR) 
        {
            continue;
        }
        // 非阻塞读的行为, 全部的数据已读取完毕(即目前的Socket缓冲区中没有数据)
        else if(nlen == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) 
        {
            // 每次读完后, 将数据以报文为单位循环发送出去
            while(true)
            {
                int len; // 获取报文头部
                memcpy(&len, _M_input_buffer.data(), 4);
                
                // 若接收缓冲区中的数据量长度小于报文(有分包现象), 则不完整, 等待
                if(_M_input_buffer.size() < len + 4)
                    break;

                // 跳过报文头, 只取报文内容
                std::string message(_M_input_buffer.data() + 4, len);
                _M_input_buffer.erase(0, len + 4); // 将该报文从缓冲区中删除

                // 打印该报文
                printf("thread id = %d, message(clnt_fd = %d): %s\n", 
                                syscall(SYS_gettid), get_fd(), message.c_str());

                _M_deal_message_callback(shared_from_this(), message);
            }

            break;
        }
        // 连接断开
        else if(nlen == 0) 
        {
            close_callback();
            break;
        }
    }
}

void Connection::write_events()
{
    // 当可写后, 尝试把用户缓冲区的数据全部发送出去
    int len = ::send(get_fd(), _M_output_buffer.data(), _M_output_buffer.size(), 0);
    // 因为操作系统的原因(tcp滑动窗口), 数据不一定能全部接收, 剩下的数据等待下一次写事件触发

    // 将成功发送的数据从用户缓冲区中删除掉
    if(len > 0) {
        _M_output_buffer.erase(0, len);
    }
    
    // 若缓冲区中没有数据了, 表示数据已成功发送, 不再关注写事件
    if(_M_output_buffer.size() == 0) {
        _M_clnt_channel_ptr->unset_write_events();
        _M_send_complete_callback(shared_from_this());
    }
}

// TcpServer::deal_message中调用
void Connection::send(const char* data, size_t size)
{  
    // 若连接已经断开
    if(_M_is_discon) {
        std::cout << "连接提前断开, send()直接返回." << std::endl;
        return;
    }

    // 先将数据发送到用户缓冲区中
    _M_output_buffer.append_with_head(data, size);

    // 注册写事件, 用来判断内核缓冲区是否可写. 若可写, Channel会回调write_events函数
    _M_clnt_channel_ptr->set_write_events();

}

Connection::~Connection()
{
    delete _M_clnt_sock_ptr;
    delete _M_clnt_channel_ptr;
}