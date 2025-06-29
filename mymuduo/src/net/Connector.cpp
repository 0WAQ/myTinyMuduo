#include "mymuduo/base/Logger.h"
#include "mymuduo/net/Connector.h"
#include "mymuduo/net/Acceptor.h"
#include "mymuduo/net/Channel.h"
#include "mymuduo/net/SocketOps.h"
#include "mymuduo/net/EventLoop.h"

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
    , _M_channel(nullptr)
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

void Connector::start_in_loop() {
    _M_loop->is_loop_thread();
    assert(_M_state == kDisConnected);

    if (_M_connect.load()) {
        connect();
    }
    else {
        LOG_DEBUG("do not connect\n");
    }
}

void Connector::restart() {
    _M_loop->is_loop_thread();
    _M_state = kDisConnected;
    _M_retry_delay = kInitRetryDelay;
    _M_connect.store(true);
    start_in_loop();
}

void Connector::stop() {
    _M_connect.store(false);
    _M_loop->queue_in_loop(std::bind(&Connector::stop_in_loop, this));
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
    int sockfd = sockets::create_non_blocking_fd();
    int ret = sockets::connect(sockfd, _M_server_addr.addr());
    int saved_errno = (ret == 0) ? 0 : errno;

    switch (saved_errno) {
    case 0:
    case EINPROGRESS:
    case EINTR:
    case EISCONN:
        do_connect(sockfd);
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

void Connector::do_connect(int sockfd) {
    _M_state = kConnecting;
    assert(!_M_channel);

    // 创建临时 channel 仅用于连接建立的过程
    _M_channel.reset(new Channel(_M_loop, sockfd));
    _M_channel->set_write_callback(std::bind(&Connector::handle_write, this));
    _M_channel->set_error_callback(std::bind(&Connector::handle_error, this));
    _M_channel->set_write_events();
}

void Connector::handle_write() {
    LOG_DEBUG("Connector::handle_write %d\n", _M_state);

    if (_M_state == kConnecting) {
        int sockfd = remove_and_reset_channel();
        int err = sockets::get_socket_error(sockfd);
        if (err != 0) {
            LOG_WARN("Connector::handle_write - SO_ERROR = %d %s\n", err, strerror(err));
            retry(sockfd);
        }
        else if(sockets::is_self_connect(sockfd)) {
            LOG_WARN("Connector::handle_write - Self connect\n");
            retry(sockfd);
        }
        else {  // 连接成功
            _M_state = kConnected;
            if (_M_connect && _M_new_connection_callback) {
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

void Connector::handle_error() {
    if (_M_state == kConnecting) {
        int sockfd = remove_and_reset_channel();
        int err = sockets::get_socket_error(sockfd);
        LOG_DEBUG("SO_ERROR = %d %s\n", err, strerror(err));
        retry(sockfd);
    }
}

void Connector::retry(int sockfd) {
    sockets::close(sockfd);
    _M_state = kDisConnected;

    if (_M_connect) {
        using namespace std::chrono;

        LOG_INFO("Connector::retry - Retry connecting to %s in %dms.\n",
                        _M_server_addr.ip_port().c_str(),
                        duration_cast<milliseconds>(_M_retry_delay).count());

        if (_M_retry_callback) {
            _M_retry_callback(sockfd);
        }

        _M_loop->run_after(_M_retry_delay,
                            std::bind(&Connector::start_in_loop, shared_from_this()));
        _M_retry_delay = std::min(_M_retry_delay * 2, kMaxRetryDelay);
    }
    else {
        LOG_DEBUG("do not connect.\n");
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
