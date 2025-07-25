#ifndef MYMUDUO_NET_INETADDRESS_H
#define MYMUDUO_NET_INETADDRESS_H

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
    
    std::string ip() const { return ::inet_ntoa(_addr.sin_addr); }
    uint16_t port() const { return ntohs(_addr.sin_port); }
    std::string ip_port() const { return ip() + ":" + std::to_string(port()); }
    sockaddr* addr() const { return (sockaddr*)&_addr; }
    void set_addr(sockaddr_in clnt_addr) { _addr = clnt_addr; }

private:
    sockaddr_in _addr;
};

} // namespace net
} // namespace mymuduo

#endif // MYMUDUO_NET_INETADDRESS_H