#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>


/**
 *  地址协议类
 *  封装构造sockaddr的过程
 */
class InetAddress
{
public:

    InetAddress();
    
    /**
     * 
     * @describe: 用ip和port初始化sockaddr_in的内部变量
     * @param:    ip: string的cv类型
     *            port: 传输主机序的port
     * 
     */
    InetAddress(const std::string& _ip, uint16_t _port);

    /**
     * 
     * @describe: 拷贝构造函数, 目前没有用到
     * @param:    const sockaddr_in
     * 
     */
    InetAddress(const sockaddr_in _addr);

    /**
     * 
     * @describe: 获取结构体的ip地址
     * @param:    void
     * @return:   const char*
     * 
     */
    const char* get_ip() const;

    /**
     * 
     * @describe: 获取结构体的port
     * @param:    void
     * @return:   主机序的port: uint16_t
     * 
     */
    uint16_t get_port() const;

    /**
     * 
     * @describe: 将内部的&sockaddr_in转换为sockaddr* 返回
     * @param:    void
     * @return:   const sockadr* 
     * 
     */
    const sockaddr* get_addr() const;

    /***
     * 
     * @describe: 服务端使用accept接收连接后, 将填充的sockaddr_in结构体设置给InetAddress类
     * @param:    sockaddr_in
     * @return:   void
     * 
     */
    void set_addr(sockaddr_in clnt_addr);


    ~InetAddress();

private:
    sockaddr_in _M_addr;
};