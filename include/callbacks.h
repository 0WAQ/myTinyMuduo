#ifndef CALLBACKS_H
#define CALLBACKS_H

#include <functional>
#include <memory>

#include "TimeStamp.h"

class Buffer;
class Connection;

using ConnectionPtr = std::shared_ptr<Connection>;
using ConnectionCallback = std::function<void(const ConnectionPtr&)>;
using CloseCallback = std::function<void(const ConnectionPtr&)>;
using WriteCompleteCallback = std::function<void(const ConnectionPtr&)>;
using TimerOutCallback = std::function<void()>; // TODO:
using MessageCallback = std::function<void(const ConnectionPtr&, Buffer*, TimeStamp)>;

#endif // CALLBACKS_H