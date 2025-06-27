#ifndef ASYNCLOGGING_H
#define ASYNCLOGGING_H

#include <chrono>
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

#include "base/LogFile.h"
#include "base/Thread.h"
#include "base/noncopyable.h"

namespace mymuduo
{

namespace __detail
{

const std::size_t kSmallBuffer = 4000;
const std::size_t kLargeBuffer = 4000*1000;

/**
 * @brief 异步日志系统的缓冲区
 */
template<int SIZE>
class Buffer : noncopyable
{
public:

    Buffer() : _M_cur(_M_data) { }
    ~Buffer() { }

    void append(const char *buf, std::size_t len) {
        if(available() > len) {
            memcpy(_M_cur, buf, len);
            _M_cur += len;
        }
    }

    const char* begin() const { return _M_data; }
    const char* end() const { return _M_data + sizeof(_M_data); }
    const char* data() const { return _M_data; }
    std::size_t size() const { return static_cast<std::size_t>(_M_cur - _M_data); }
    std::size_t length() const { return size(); }
    std::size_t available() const { return static_cast<std::size_t>(end() - _M_cur); }
    bool empty() const { return _M_data == _M_cur; }
    std::string to_string() const { return std::string(_M_data, size()); }

    char* curr() { return _M_cur; }
    void add(std::size_t len) { _M_cur += len; }
    void reset() { _M_cur = _M_data; }
    void bzero() { ::bzero(_M_data, sizeof(_M_data)); }

private:
    char _M_data[SIZE];
    char *_M_cur;
};

} // namespace __detail

/**
 * @brief 异步日志的后端线程
 */
class AsyncLogging : noncopyable
{
public:

    AsyncLogging(const std::filesystem::path& filepath,
                    const std::string& filename,
                    off_t roll_size = 500*1000*1000,
                    std::chrono::seconds flush_interval = std::chrono::seconds { 3 });

    ~AsyncLogging();

    /**
     * @brief 会被前端线程写日志时调用
     */
    void append(const char *logline, std::size_t len);
    
    void start();
    void stop();

private:
    
    /**
     * @brief 后端线程的任务 
     */
    void thread_func();

    using Buffer = __detail::Buffer<__detail::kLargeBuffer>;
    using BufferPtr = std::unique_ptr<Buffer>;
    using BufferVector = std::vector<BufferPtr>;

private:

    std::atomic<bool> _M_running;

    const std::chrono::seconds _M_flush_interval;    // 刷新缓冲区的间隔时间
    std::unique_ptr<LogFile> _M_log_file;

    Thread _M_thread;               // 后端线程
   
    std::counting_semaphore<1> _M_sem; // 用于等待后端线程的初始化
    std::mutex _M_mutex;               // 在append上加锁(有多个前端线程), 用于保护下面四个变量
    std::condition_variable _M_cond;   // 后端线程阻塞在该条件变量上

    BufferPtr _M_curr_buffer;       // 当前缓冲
    BufferPtr _M_next_buffer;       // 预备缓冲
    BufferVector _M_buffers;        // 待落入磁盘的以添满的缓冲
};

} // namespace mymuduo

#endif // ASYNCLOGGING_H
