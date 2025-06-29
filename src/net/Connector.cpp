#include "base/Logger.h"
#include "net/Connector.h"
#include "net/Acceptor.h"
#include "net/Channel.h"
#include "net/Socket.h"
#include "net/SocketOps.h"

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
        sockets::close(sockfd);
        break;

    default:
        LOG_ERROR("Unexpected error in Connector::start_in_loop %d\n", saved_errno);
        sockets::close(sockfd);
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

        int sockfd = remove_and_reset_channel();
        int err = get_socket_error(sockfd);
        if (err) {
            LOG_WARN("Connector::handle_write - SO_ERROR = %d %s\n", err, strerror(err));
            retry(sockfd);
        }
        else if(sockets::is_self_connect(sockfd)) {
            LOG_WARN("Connector::handle_write - Self connect\n");
            retry(sockfd);
        }
        else {
            _M_state = kConnected;
            if (_M_connect) {
                _M_new_connection_callback(sockfd);
            }
            else {
                sockets::close(sockfd);
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
    sockets::close(sockfd);
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
