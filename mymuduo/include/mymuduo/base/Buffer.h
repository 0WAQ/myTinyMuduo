#ifndef MYMUDUO_BASE_BUFFER
#define MYMUDUO_BASE_BUFFER

#include <string>
#include <cstring>

#include "mymuduo/base/noncopyable.h"

namespace mymuduo {
namespace __detail {

const std::size_t kSmallBuffer = 4*1024;
const std::size_t kLargeBuffer = 4000*1024;

/**
 * @brief 异步日志系统的缓冲区
 */
template<size_t SIZE>
class Buffer : noncopyable {
public:
    Buffer() : _cur(_data) { }
    ~Buffer() { }

    void append(const char *buf, std::size_t len) {
        if(available() >= len) {
            memcpy(_cur, buf, len);
            _cur += len;
        }
    }

    constexpr char* begin() const { return _data; }
    const char* end() const { return _data + sizeof(_data); }
    const char* data() const { return _data; }
    size_t size() const { return static_cast<size_t>(_cur - _data); }
    size_t length() const { return size(); }
    size_t available() const { return static_cast<size_t>(end() - _cur); }
    static constexpr size_t capacity() { return SIZE; }
    bool empty() const { return _data == _cur; }
    std::string to_string() const { return std::string(_data, size()); }

    char* curr() { return _cur; }
    void add(size_t len) { _cur += len; }
    void reset() { _cur = _data; }
    void bzero() { ::bzero(_data, sizeof(_data)); }

private:
    char _data[SIZE];
    char *_cur;
};

} // namespace __detail
} // namespace mymuduo

#endif // MYMUDUO_BASE_BUFFER