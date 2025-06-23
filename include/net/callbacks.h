#ifndef CALLBACKS_H
#define CALLBACKS_H

#include <functional>
#include <memory>

#include "base/TimeStamp.h"

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
using MessageCallback = std::function<void(const TcpConnectionPtr&, Buffer*, TimeStamp)>;

using TimerCallback = std::function<void()>;
using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr&, size_t)>;

} // namespace net
} // namespace mymuduo

#endif // CALLBACKS_H