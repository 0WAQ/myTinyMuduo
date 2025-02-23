/**
 * 
 * InetAddress头文件
 * 
 */
#ifndef INETADDRESS_H
#define INETADDRESS_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <strings.h>

class InetAddress
{
public:
    
    InetAddress() = default;

    /**
     * @brief 用ip和port初始化sockaddr_in的内部变量 
     * @param port 接收主机序的port
     */
    explicit InetAddress(const std::string& ip, uint16_t port);

    explicit InetAddress(const sockaddr_in addr) : _M_addr(addr) { }

    std::string get_ip() const;

    uint16_t get_port() const;

    const sockaddr* get_addr() const;

    /**
     * @brief 服务端使用accept接收连接后, 将填充好的sockaddr_in结构体设置给InetAddress
     */
    void set_addr(sockaddr_in clnt_addr);

private:
    sockaddr_in _M_addr;
};

#endif // INETADDRESS_H