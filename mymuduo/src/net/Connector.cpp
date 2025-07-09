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
    : _loop(loop)
    , _server_addr(server_addr)
    , _channel(nullptr)
    , _connect(false)
    , _state(kDisConnected)
    , _retry_delay(kInitRetryDelay)
{
    LOG_DEBUG("ctor[{}]", (void*)this);
}

Connector::~Connector() {
    LOG_DEBUG("dtor[{}]", (void*)this);
    assert(!_channel);
}

void Connector::start() {
    _connect.store(true);
    _loop->run_in_loop(std::bind(&Connector::start_in_loop, this));
}

void Connector::start_in_loop() {
    _loop->is_loop_thread();
    assert(_state == kDisConnected);

    if (_connect.load()) {
        connect();
    }
    else {
        LOG_DEBUG("do not connect");
    }
}

void Connector::restart() {
    _loop->is_loop_thread();
    _state = kDisConnected;
    _retry_delay = kInitRetryDelay;
    _connect.store(true);
    start_in_loop();
}

void Connector::stop() {
    if (_connect.load()) {
        _connect.store(false);
        _loop->queue_in_loop(std::bind(&Connector::stop_in_loop, this));
    }
}


void Connector::stop_in_loop() {
    _loop->is_loop_thread();
    if (_state == kConnecting) {
        _state = kDisConnected;
        int sockfd = remove_and_reset_channel();
        retry(sockfd);
    }
}

void Connector::connect() {
    int sockfd = sockets::create_non_blocking_fd();
    int ret = sockets::connect(sockfd, _server_addr.addr());
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
        LOG_ERROR("connect error in Connector::start_in_loop {}", saved_errno);
        sockets::close(sockfd);
        break;

    default:
        LOG_ERROR("Unexpected error in Connector::start_in_loop {}", saved_errno);
        sockets::close(sockfd);
        break;
    }
}

void Connector::do_connect(int sockfd) {
    _state = kConnecting;
    assert(!_channel);

    // 创建临时 channel 仅用于连接建立的过程
    _channel.reset(new Channel(_loop, sockfd));
    _channel->set_write_callback(std::bind(&Connector::handle_write, this));
    _channel->set_error_callback(std::bind(&Connector::handle_error, this));
    _channel->set_write_events();
}

void Connector::handle_write() {
    LOG_DEBUG("Connector::handle_write {}", (int)_state);

    if (_state == kConnecting) {
        int sockfd = remove_and_reset_channel();
        int err = sockets::get_socket_error(sockfd);
        if (err != 0) {
            LOG_WARN("Connector::handle_write - SO_ERROR = {} {}", err, strerror(err));
            retry(sockfd);
        }
        else if(sockets::is_self_connect(sockfd)) {
            LOG_WARN("Connector::handle_write - Self connect");
            retry(sockfd);
        }
        else {  // 连接成功
            _state = kConnected;
            if (_connect && _new_connection_callback) {
                _new_connection_callback(sockfd);
            }
            else {
                sockets::close(sockfd);
            }
        }
    }
    else {
        assert(_state == kDisConnected);
    }
}

void Connector::handle_error() {
    if (_state == kConnecting) {
        int sockfd = remove_and_reset_channel();
        int err = sockets::get_socket_error(sockfd);
        LOG_DEBUG("SO_ERROR = {} {}", err, strerror(err));
        retry(sockfd);
    }
}

void Connector::retry(int sockfd) {
    sockets::close(sockfd);
    _state = kDisConnected;

    if (_connect) {
        using namespace std::chrono;

        LOG_INFO("Connector::retry - Retry connecting to {} in {} ms.",
                        _server_addr.ip_port(),
                        duration_cast<milliseconds>(_retry_delay).count());

        if (_retry_callback) {
            _retry_callback(sockfd);
        }

        _loop->run_after(_retry_delay,
                            std::bind(&Connector::start_in_loop, shared_from_this()));
        _retry_delay = std::min(_retry_delay * 2, kMaxRetryDelay);
    }
    else {
        LOG_DEBUG("do not connect.");
    }
}

int Connector::remove_and_reset_channel() {
    _channel->unset_all_events();
    _channel->remove();

    int sockfd = _channel->fd();
    _loop->queue_in_loop(std::bind(&Connector::reset_channel, this));
    return sockfd;
}

void Connector::reset_channel() {
    _channel.reset();
}
