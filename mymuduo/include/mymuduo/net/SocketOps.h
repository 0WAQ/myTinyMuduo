#ifndef SOCKETOPS_H
#define SOCKETOPS_H

#include <arpa/inet.h>

namespace mymuduo {
namespace net {
namespace sockets {

void close(int sockfd);

sockaddr_in get_local_addr(int sockfd);
sockaddr_in get_peer_addr(int sockfd);

bool is_self_connect(int sockfd);

} // namespace sockets
} // namespace net
} // namespace mymuduo

#endif // SOCKETOPS_H
