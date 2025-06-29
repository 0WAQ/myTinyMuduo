/**
 * 
 * Socket头文件
 * 
 */
#ifndef SOCKET_H
#define SOCKET_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <cerrno>
#include <unistd.h>

#include "mymuduo/base/noncopyable.h"

namespace mymuduo {
namespace net {

class InetAddress;

/**
 *  封装socket
 */
class Socket : noncopyable
{
public:

    explicit Socket(int fd) : _M_fd(fd) { }
    
    ~Socket() {
        if (_M_closed) {
            return;
        }
        close();
    }

    void bind(const InetAddress& serv_addr);
    void listen(size_t max_connection = 1024);
    int accept(InetAddress& clnt_addr);
    void close();

    /**
     * @brief 设置半关闭
     */
    void shutdown_write();

    /**
     * @brief socket属性
     */
    void set_reuse_addr(bool on);   // 地址重用
    void set_reuse_port(bool on);   // 端口重用
    void set_tcp_nodelay(bool on);  // 不启用naggle算法
    void set_keep_alive(bool on);   // 保持连接

    int fd() const { return _M_fd; }

private:
    const int _M_fd;
    bool _M_closed = false;
};

} // namespace net
} // namespace mymuduo

#endif // SOCKET_H