#ifndef MYMUDUO_BASE_LOGFORMAT_H
#define MYMUDUO_BASE_LOGFORMAT_H

#include "mymuduo/base/noncopyable.h"
#include "mymuduo/base/Buffer.h"

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <format>

namespace mymuduo {

class Fmt {
public:
    template <typename T>
    Fmt(const char* fmt, T val)
        : _len(val)
    {
        _len = std::snprintf(_buf, sizeof(_buf), fmt, val);
        assert(_len < sizeof(_buf));
    }

    const char* data() const { return _buf; }
    size_t size() const { return _len; }

private:
    char _buf[32];
    size_t _len;
};

class T {
public:
    T(const char* data, size_t len)
        : _str(data)
        , _len(len)
    {
        assert(len == strlen(_str));
    }

    const char* _str;
    const size_t _len;
};

class LogFormat : noncopyable {
public:
    using Buffer = __detail::Buffer<__detail::kSmallBuffer>;

public:

    // 提供格式化输出
    template <typename... Args>
    void format(std::format_string<Args...> fmt, Args... args) {
        char* it = std::format_to(_buffer.curr(), fmt, std::forward<Args>(args)...);
        _buffer.add(static_cast<size_t>(it - _buffer.curr()));
    }

    // 提供流式输出
    LogFormat& operator<< (const std::string& str);
    LogFormat& operator<< (const char* str);
    LogFormat& operator<< (T str);
    LogFormat& operator<< (char c);

    const Buffer& buffer() const noexcept { return _buffer; }

private:
    Buffer _buffer;
};

} // namespace mymuduo

#endif // MYMUDUO_BASE_LOGFORMAT_H
