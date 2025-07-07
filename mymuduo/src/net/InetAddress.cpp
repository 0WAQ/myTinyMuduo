#include "mymuduo/net/InetAddress.h"

using namespace mymuduo;
using namespace mymuduo::net;

InetAddress::InetAddress(uint16_t port)
    : InetAddress("0.0.0.0", port)
{ }

InetAddress::InetAddress(const std::string& ip, uint16_t port) {
    bzero(&_addr, sizeof(sockaddr_in));
    _addr.sin_family = AF_INET;
    _addr.sin_addr.s_addr = inet_addr(ip.c_str());
    _addr.sin_port = htons(port);
}

InetAddress::InetAddress(const char* ip, const char* port) {
    _addr.sin_family = AF_INET;
    _addr.sin_addr.s_addr = inet_addr(ip);
    _addr.sin_port = htons((uint16_t)atoi(port));
}

InetAddress::InetAddress(const sockaddr_in addr)
    : _addr(addr)
{ }

InetAddress& InetAddress::operator= (const sockaddr_in& addr) {
    _addr = addr;
    return *this;
}
