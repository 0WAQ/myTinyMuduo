#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>


class InetAddress
{
public:


    InetAddress();


    /// @brief 用ip和port初始化sockaddr_in的内部变量 
    /// @param ip string的cv类型
    /// @param port 接收主机序的port
    InetAddress(const std::string& ip, uint16_t port);


    /// @brief 拷贝构造
    /// @param addr 
    InetAddress(const sockaddr_in addr);


    /// @brief 返回ip, port, addr
    const char* get_ip() const;
    uint16_t get_port() const;
    const sockaddr* get_addr() const;


    /// @brief 服务端使用accept接收连接后, 将填充的sockaddr_in结构体设置给InetAddress类
    /// @param clnt_addr 客户端地址结构体
    void set_addr(sockaddr_in clnt_addr);

private:
    sockaddr_in _M_addr;
};