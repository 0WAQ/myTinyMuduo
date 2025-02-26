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

    /**
     * @brief 用ip和port初始化sockaddr_in的内部变量 
     * @param port 接收主机序的port
     */
    explicit InetAddress(const std::string& ip = {}, uint16_t port = 0);
    explicit InetAddress(const sockaddr_in addr) : _M_addr(addr) { }


    std::string get_ip() const { return ::inet_ntoa(_M_addr.sin_addr); }
    uint16_t get_port() const { return ntohs(_M_addr.sin_port); }
    std::string get_ip_port() const { return get_ip() + ":" + std::to_string(get_port()); }
    const sockaddr* get_addr() const { return (sockaddr*)&_M_addr; }
    void set_addr(sockaddr_in clnt_addr) { _M_addr = clnt_addr; }

private:
    sockaddr_in _M_addr;
};

#endif // INETADDRESS_H