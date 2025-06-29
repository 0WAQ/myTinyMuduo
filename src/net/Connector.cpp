#include "base/Logger.h"
#include "net/Connector.h"
#include "net/Acceptor.h"
#include "net/Channel.h"
#include "net/Socket.h"

#include <asm-generic/socket.h>
#include <cassert>
#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace mymuduo;
using namespace mymuduo::net;

Connector::Connector(EventLoop* loop, const InetAddress& server_addr) 
    : _M_loop(loop)
    , _M_server_addr(server_addr)
    , _M_connect(false)
    , _M_state(kDisConnected)
    , _M_retry_delay(kInitRetryDelay)
{
    LOG_DEBUG("ctor[%x]\n", this);
}

Connector::~Connector() {
    LOG_DEBUG("dtor[%x]\n", this);
    assert(!_M_channel);
}

void Connector::start() {
    _M_connect.store(true);
    _M_loop->run_in_loop(std::bind(&Connector::start_in_loop, this));
}

void Connector::restart() {
    _M_loop->is_loop_thread();
    _M_state= kDisConnected;
    _M_retry_delay = kInitRetryDelay;
    _M_connect.store(true);
    start_in_loop();
}

void Connector::stop() {
    _M_connect.store(false);
    _M_loop->queue_in_loop(std::bind(&Connector::stop_in_loop, this));
}


void Connector::start_in_loop() {
    _M_loop->is_loop_thread();
    assert(_M_state = kDisConnected);

    if (_M_connect) {
        connect();
    }
    else {
        LOG_DEBUG("do not connector\n");
    }
}

void Connector::stop_in_loop() {
    _M_loop->is_loop_thread();
    if (_M_state == kConnecting) {
        _M_state = kDisConnected;
        int sockfd = remove_and_reset_channel();
        retry(sockfd);
    }
}

void Connector::connect() {
    int sockfd = __detail::create_non_blocking_fd();
    int ret = ::connect(sockfd, _M_server_addr.addr(), sizeof(sockaddr));
    int saved_errno = (ret == 0) ? 0 : errno;

    switch (saved_errno) {
    case 0:
    case EINPROGRESS:
    case EINTR:
    case EISCONN:
        connecting(sockfd);
        break;

    case EAGAIN:
    case EADDRINUSE:
    case EADDRNOTAVAIL:
    case ECONNREFUSED:
    case ENETUNREACH:
        retry(sockfd);
        break;

    case EACCES:
    case EPERM:
    case EAFNOSUPPORT:
    case EALREADY:
    case EBADF:
    case EFAULT:
    case ENOTSOCK:
        LOG_ERROR("connect error in Connector::start_in_loop %d\n", saved_errno);
        if (::close(sockfd) < 0) {
            LOG_ERROR("%s:%s:%d close error:%d.\n",
                __FILE__, __FUNCTION__, __LINE__, errno);
        }
        break;

    default:
        LOG_ERROR("Unexpected error in Connector::start_in_loop %d\n", saved_errno);
        if (::close(sockfd) < 0) {
            LOG_ERROR("%s:%s:%d close error:%d.\n",
                __FILE__, __FUNCTION__, __LINE__, errno);
        }
        break;
    }
}

void Connector::connecting(int sockfd) {
    _M_state = kConnecting;
    assert(!_M_channel);
    _M_channel.reset(new Channel(_M_loop, sockfd));
    _M_channel->set_write_callback(std::bind(&Connector::handle_write, this));
    _M_channel->set_error_callback(std::bind(&Connector::handle_error, this));
    _M_channel->set_write_events();
}

// TODO:
void Connector::handle_write() {
    LOG_DEBUG("Connector::handle_write %d\n", _M_state);

    if (_M_state == kConnecting) {

        auto get_socket_error = [](int sockfd) {
            int optval;
            socklen_t optlen = static_cast<socklen_t>(sizeof(optval));

            if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
                return errno;
            }
            else {
                return optval;
            }
        };

        auto is_self_connect = [](int sockfd) {
            struct sockaddr_in6 localaddr = [](int sockfd) {
                struct sockaddr_in6 localaddr;
                bzero(&localaddr, sizeof localaddr);
                socklen_t addrlen = static_cast<socklen_t>(sizeof localaddr);
                if (::getsockname(sockfd, static_cast<struct sockaddr*>(static_cast<void*>(&localaddr)), &addrlen) < 0) {
                    LOG_ERROR("is_self_connect");
                }
                return localaddr;
            }(sockfd);

            struct sockaddr_in6 peeraddr = [](int sockfd) {
                struct sockaddr_in6 peeraddr;
                bzero(&peeraddr, sizeof peeraddr);
                socklen_t addrlen = static_cast<socklen_t>(sizeof peeraddr);
                if (::getpeername(sockfd, static_cast<struct sockaddr*>(static_cast<void*>(&peeraddr)), &addrlen) < 0) {
                    LOG_ERROR("is_self_connect");
                }
                return peeraddr;
            }(sockfd);

            if (localaddr.sin6_family == AF_INET) {
                const struct sockaddr_in* laddr4 = reinterpret_cast<struct sockaddr_in*>(&localaddr);
                const struct sockaddr_in* raddr4 = reinterpret_cast<struct sockaddr_in*>(&peeraddr);
                return laddr4->sin_port == raddr4->sin_port
                    && laddr4->sin_addr.s_addr == raddr4->sin_addr.s_addr;
            }
            else if (localaddr.sin6_family == AF_INET6) {
                return localaddr.sin6_port == peeraddr.sin6_port
                    && memcmp(&localaddr.sin6_addr, &peeraddr.sin6_addr, sizeof localaddr.sin6_addr) == 0;
            }
            else {
                return false;
            }
        };

        int sockfd = remove_and_reset_channel();
        int err = get_socket_error(sockfd);
        if (err) {
            LOG_WARN("Connector::handle_write - SO_ERROR = %d %s\n", err, strerror(err));
            retry(sockfd);
        }
        else if(is_self_connect(sockfd)) {
            LOG_WARN("Connector::handle_write - Self connect\n");
            retry(sockfd);
        }
        else {
            _M_state = kConnected;
            if (_M_connect) {
                _M_new_connection_callback(sockfd);
            }
            else {
                ::close(sockfd);
            }
        }
    }
    else {
        assert(_M_state == kDisConnected);
    }
}

// TODO:
void Connector::handle_error() {
    if (_M_state == kConnecting) {
        int sockfd = remove_and_reset_channel();
        
        auto get_socket_error = [](int sockfd) {
            int optval;
            socklen_t optlen = static_cast<socklen_t>(sizeof(optval));

            if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
                return errno;
            }
            else {
                return optval;
            }
        };

        int err = get_socket_error(sockfd);
        LOG_DEBUG("SO_ERROR = %d %s\n", err, strerror(err));
        retry(sockfd);
    }
}

void Connector::retry(int sockfd) {
    if (::close(sockfd) < 0) {
        LOG_ERROR("%s:%s:%d close error:%d.\n",
            __FILE__, __FUNCTION__, __LINE__, errno);
    }
}

int Connector::remove_and_reset_channel() {
    _M_channel->unset_all_events();
    _M_channel->remove();

    int sockfd = _M_channel->fd();
    _M_loop->queue_in_loop(std::bind(&Connector::reset_channel, this));
    return sockfd;
}

void Connector::reset_channel() {
    _M_channel.reset();
}
