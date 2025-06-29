#ifndef INETADDRESS_H
#define INETADDRESS_H

#include <netinet/in.h>
#include <string>
#include <strings.h>
#include <arpa/inet.h>

namespace mymuduo {
namespace net {

class InetAddress {
public:
    explicit InetAddress(uint16_t port);
    explicit InetAddress(const std::string& ip = {}, uint16_t port = 0);
    explicit InetAddress(const char* ip, const char* port);
    explicit InetAddress(const sockaddr_in addr);

    InetAddress& operator= (const sockaddr_in& addr);
    
    std::string ip() const { return ::inet_ntoa(_M_addr.sin_addr); }
    uint16_t port() const { return ntohs(_M_addr.sin_port); }
    std::string ip_port() const { return ip() + ":" + std::to_string(port()); }
    sockaddr* addr() const { return (sockaddr*)&_M_addr; }
    void set_addr(sockaddr_in clnt_addr) { _M_addr = clnt_addr; }

private:
    sockaddr_in _M_addr;
};

} // namespace net
} // namespace mymuduo

#endif // INETADDRESS_H