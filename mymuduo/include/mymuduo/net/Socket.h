#ifndef MYMUDUO_NET_SOCKET_H
#define MYMUDUO_NET_SOCKET_H

#include <netinet/tcp.h>

#include "mymuduo/base/noncopyable.h"

namespace mymuduo {
namespace net {

class InetAddress;

class Socket : noncopyable
{
public:
    explicit Socket(int fd);
    ~Socket();

    void bind(const InetAddress& serv_addr);
    void listen();
    int accept(InetAddress& clnt_addr);
    void close();

    /**
     * @brief 半关闭
     */
    void shutdown_write();

    /**
     * @brief socket属性
     */
    void set_reuse_addr(bool on);   // 地址重用
    void set_reuse_port(bool on);   // 端口重用
    void set_tcp_nodelay(bool on);  // 不启用naggle算法
    void set_keep_alive(bool on);   // 保持连接

    int fd() const { return _fd; }

private:
    const int _fd;
    bool _closed = false;
};

} // namespace net
} // namespace mymuduo

#endif // MYMUDUO_NET_SOCKET_H