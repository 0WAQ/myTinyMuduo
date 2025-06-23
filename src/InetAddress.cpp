#include "InetAddress.h"
#include "Logger.h"
#include <arpa/inet.h>
#include <cstdint>
#include <netinet/in.h>
#include <strings.h>
#include <sys/socket.h>

namespace mymuduo
{

InetAddress::InetAddress(uint16_t port)
    : InetAddress("0.0.0.0", port)
{ }

InetAddress::InetAddress(const std::string& ip, uint16_t port) {
    bzero(&_M_addr, sizeof(sockaddr_in));
    _M_addr.sin_family = AF_INET;
    _M_addr.sin_addr.s_addr = inet_addr(ip.c_str());
    _M_addr.sin_port = htons(port);
}

InetAddress::InetAddress(const char* ip, const char* port) {
    _M_addr.sin_family = AF_INET;
    _M_addr.sin_addr.s_addr = inet_addr(ip);
    _M_addr.sin_port = htons((uint16_t)atoi(port));
}

sockaddr_in InetAddress::get_local_addr(int sockfd)
{
    sockaddr_in local;
    ::bzero(&local, sizeof(sockaddr_in));
    socklen_t len = sizeof(sockaddr_in);
    if(::getsockname(sockfd, (sockaddr*)&local, &len) < 0) {
        LOG_ERROR("InetAddress::get_local_addr.\n");
    }
    return local;
}

sockaddr_in InetAddress::get_peer_addr(int sockfd)
{
    sockaddr_in peer;
    ::bzero(&peer, sizeof(sockaddr_in));
    socklen_t len = sizeof(sockaddr_in);
    if(::getpeername(sockfd, (sockaddr*)&peer, &len) < 0) {
        LOG_ERROR("InetAddress::get_peer_addr,\n");
    }
    return peer;
}

} // namespace mymuduo