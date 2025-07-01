#ifndef CALLBACKS_H
#define CALLBACKS_H

#include <functional>
#include <memory>

#include "mymuduo/base/Timestamp.h"

namespace mymuduo {
namespace net {

class Buffer;
class EventLoop;
class TcpConnection;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

using ThreadInitCallback = std::function<void(EventLoop*)>;

using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
using CloseCallback = std::function<void(const TcpConnectionPtr&)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;
using MessageCallback = std::function<void(const TcpConnectionPtr&, Buffer*, Timestamp)>;

using TimerCallback = std::function<void()>;
using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr&, size_t)>;

void default_connection_callback(const TcpConnectionPtr& conn);
void default_message_callback(const TcpConnectionPtr& conn, Buffer* buf, Timestamp t);

} // namespace net
} // namespace mymuduo

#endif // CALLBACKS_H