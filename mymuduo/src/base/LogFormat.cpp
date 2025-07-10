#include "mymuduo/base/LogFormat.h"

using namespace mymuduo;

LogFormat& LogFormat::operator<< (const std::string& str) {
    if (!str.empty()) {
        _buffer.append(str.data(), str.size());
    }
    else {
        _buffer.append("(null)", 6);
    }
    return *this;
}

LogFormat& LogFormat::operator<< (const char* str) {
    if (str) {
        _buffer.append(str, strlen(str));
    }
    else {
        _buffer.append("(null)", 6);
    }
    return *this;
}

LogFormat& LogFormat::operator<< (T str) {
    _buffer.append(str._str, str._len);
    return *this;
}

LogFormat& LogFormat::operator<< (char c) {
    _buffer.append(c);
    return *this;
}