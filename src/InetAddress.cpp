#include "InetAddress.h"

InetAddress::InetAddress(const std::string& ip, uint16_t port) {
    bzero(&_M_addr, sizeof(sockaddr_in));
    _M_addr.sin_family = AF_INET;
    _M_addr.sin_addr.s_addr = inet_addr(ip.c_str());
    _M_addr.sin_port = htons(port);
}
