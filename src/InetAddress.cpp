#include "InetAddress.h"

InetAddress::InetAddress(const std::string& ip, uint16_t port) {
    bzero(&_M_addr, sizeof(sockaddr_in));
    _M_addr.sin_family = AF_INET;
    _M_addr.sin_addr.s_addr = inet_addr(ip.c_str());
    _M_addr.sin_port = htons(port);
}

std::string InetAddress::get_ip() const {
    return ::inet_ntoa(_M_addr.sin_addr);
}

uint16_t InetAddress::get_port() const {
    return ntohs(_M_addr.sin_port);
}

const sockaddr* InetAddress::get_addr() const { 
    return (sockaddr*)&_M_addr;
}

void InetAddress::set_addr(sockaddr_in clnt_addr) {
    _M_addr = clnt_addr;
}
