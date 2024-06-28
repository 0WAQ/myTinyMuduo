#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>


/**
 *  package the process of filling the structre which 
 *  includes the ip, the port and the family 
 */
class InetAddress
{
public:

    InetAddress();
    
    /**
     * 
     * @pragma: _ip: target ip
     *          _port: target port which needs network order
     * 
     */
    InetAddress(const std::string& _ip, uint16_t _port);

    /**
     * 
     * @pragma: _addr: init to _M_addr
     * 
     */
    InetAddress(const sockaddr_in _addr);

    /**
     * 
     * @return: ip type of const char* of the _M_addr
     * 
     */
    const char* ip() const;

    /**
     * 
     * @return: port(host order) type of uint16_t of the _M_addr
     * 
     */
    uint16_t port() const;

    /**
     * 
     * @return: _M_addr's sockaddr
     * 
     */
    const sockaddr* addr() const;

    /***
     * 
     * 
     * 
     */
    void set_addr(sockaddr_in clnt_addr);

    ~InetAddress();

private:
    sockaddr_in _M_addr;
};