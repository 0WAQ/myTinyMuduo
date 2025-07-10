#ifndef MYMUDUO_BASE_BUFFER
#define MYMUDUO_BASE_BUFFER

#include <string>
#include <cstring>

#include "mymuduo/base/noncopyable.h"

namespace mymuduo {
namespace __detail {

const std::size_t kSmallBuffer = 4*1000;
const std::size_t kLargeBuffer = 4000*1000;

/**
 * @brief 日志系统的缓冲区
 */
template<size_t SIZE>
class Buffer : noncopyable {
public:
    Buffer() : _cur(_data) { }
    ~Buffer() { }

    inline void append(const char *buf, std::size_t len) {
        if(available() >= len) {
            memcpy(_cur, buf, len);
            _cur += len;
        }
    }

    inline void append(char c) {
        if (available() >= 1) {
            *_cur = c;
            _cur += 1;
        }
    }

    inline constexpr char* begin() const { return _data; }
    inline const char* end() const { return _data + sizeof(_data); }
    inline const char* data() const { return _data; }
    inline size_t size() const { return static_cast<size_t>(_cur - _data); }
    inline size_t length() const { return size(); }
    inline size_t available() const { return static_cast<size_t>(end() - _cur); }
    inline static constexpr size_t capacity() { return SIZE; }
    inline bool empty() const { return _data == _cur; }
    inline std::string to_string() const { return std::string(_data, size()); }

    inline char* curr() { return _cur; }
    inline void add(size_t len) { _cur += len; }
    inline void reset() { _cur = _data; }
    inline void bzero() { ::bzero(_data, sizeof(_data)); }

private:
    char _data[SIZE];
    char *_cur;
};

} // namespace __detail
} // namespace mymuduo

#endif // MYMUDUO_BASE_BUFFER