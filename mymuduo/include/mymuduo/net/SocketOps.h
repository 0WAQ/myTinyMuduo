#ifndef SOCKETOPS_H
#define SOCKETOPS_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

namespace mymuduo {
namespace net {
namespace sockets {

int create_non_blocking_fd();
int setsockopt(int sockfd, int level, int optname, bool on);

void bind(int sockfd, struct sockaddr* addr);
void listen(int sockfd);
int accept(int listenfd, struct sockaddr* addr);
int connect(int sockfd, struct sockaddr* addr);
void close(int sockfd);
void shutdown_write(int sockfd);

int get_socket_error(int sockfd);

sockaddr_in get_local_addr(int sockfd);
sockaddr_in get_peer_addr(int sockfd);

bool is_self_connect(int sockfd);

} // namespace sockets
} // namespace net
} // namespace mymuduo

#endif // SOCKETOPS_H
