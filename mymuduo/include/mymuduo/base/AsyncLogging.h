#ifndef MYMUDUO_BASE_ASYNCLOGGING_H
#define MYMUDUO_BASE_ASYNCLOGGING_H

#include <chrono>
#include <cstddef>
#include <string>
#include <filesystem>
#include <cstring>
#include <strings.h>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <semaphore>
#include <condition_variable>

#include "mymuduo/base/LogFile.h"
#include "mymuduo/base/Thread.h"
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


class AsyncLogging : noncopyable {
public:
    using Buffer = __detail::Buffer<__detail::kLargeBuffer>;
    using BufferPtr = std::unique_ptr<Buffer>;
    using BufferVector = std::vector<BufferPtr>;

public:
    AsyncLogging(const std::filesystem::path& filepath,
                    const std::string& filename,
                    off_t roll_size = 500*1000*1000,
                    std::chrono::seconds flush_interval = std::chrono::seconds { 3 });

    AsyncLogging(std::unique_ptr<LogFile> log_file);
    ~AsyncLogging();

    /**
     * @brief 会被前端线程调用
     */
    void append(const char *logline, std::size_t len);
    
    void start();
    void stop();

private:
    void thread_func();

private:
    std::atomic<bool> _running;

    const std::chrono::seconds _flush_interval;    // 刷新缓冲区的间隔时间
    std::unique_ptr<LogFile> _log_file;

    Thread _thread;               // 后端线程
   
    std::counting_semaphore<1> _sem; // 用于等待后端线程的初始化
    std::mutex _mutex;               // 在append上加锁(有多个前端线程), 用于保护下面四个变量
    std::condition_variable _cond;   // 后端线程阻塞在该条件变量上

    BufferPtr _curr_buffer;       // 当前缓冲
    BufferPtr _next_buffer;       // 预备缓冲
    BufferVector _buffers;        // 已写满的缓冲, 由后端线程落入磁盘
};

} // namespace mymuduo

#endif // MYMUDUO_BASE_ASYNCLOGGING_H
