#include "AsyncLogging.h"
#include "LogFile.h"
#include "TimeStamp.h"
#include <unistd.h>
#include <cassert>
#include <chrono>

namespace mymuduo
{
AsyncLogging::AsyncLogging(const std::string& basename, off_t roll_size, int flush_interval) :
        _M_running(false),
        _M_basename(basename),
        _M_flush_interval(flush_interval),
        _M_roll_size(roll_size),
        _M_thread(std::bind(&AsyncLogging::thread_func, this), "Logging"),
        _M_sem(0),
        _M_mutex(),
        _M_cond(),
        _M_curr_buffer(new Buffer),
        _M_next_buffer(new Buffer),
        _M_buffers()
{
    _M_curr_buffer->bzero();
    _M_next_buffer->bzero();
    _M_buffers.reserve(16);
}

void AsyncLogging::append(const char *logline, std::size_t len)
{
    std::lock_guard<std::mutex> guard(_M_mutex);
    
    // 若当前缓冲剩余空间足够大
    if(_M_curr_buffer->available() > len)
    {
        _M_curr_buffer->append(logline, len);
    }
    else
    {
        // 若前端Buffer缓冲区不够该条日志了, 那么将其放入BufferVector中
        _M_buffers.emplace_back(_M_curr_buffer.release());

        if(_M_next_buffer)
        {
            // 将next_buffer移动到curr_buffer
            _M_curr_buffer = std::move(_M_next_buffer);
        }
        else    // 若next_buffer不存在, 则分配一个新的
        {
            // 让curr_buffer管理一个新的缓冲区
            _M_curr_buffer.reset(new Buffer);
        }

        // 写入buffer
        _M_curr_buffer->append(logline, len);

        // 唤醒后端线程
        _M_cond.notify_one();
    }
}

void AsyncLogging::thread_func()
{
    _M_sem.release();
    LogFile file(_M_basename, _M_roll_size, false);


    BufferPtr new_buffer1(new Buffer), new_buffer2(new Buffer);
    new_buffer1->bzero(), new_buffer2->bzero();

    BufferVector buffers_to_write;
    buffers_to_write.reserve(16);

    while(_M_running)
    {
        assert(new_buffer1 && new_buffer1->size() == 0);
        assert(new_buffer2 && new_buffer2->size() == 0);
        assert(buffers_to_write.empty());

        {
            std::unique_lock<std::mutex> lock(_M_mutex);

            // 如果暂且没有待落盘的缓冲区, 则释放锁并且等待flush_interval秒
            if(_M_buffers.empty()) {
                _M_cond.wait_for(lock, std::chrono::seconds{_M_flush_interval});
            }

            _M_buffers.emplace_back(std::move(_M_curr_buffer));
            _M_curr_buffer = std::move(new_buffer1);
            if(!_M_next_buffer) {
                _M_next_buffer = std::move(new_buffer2);
            }

            buffers_to_write.swap(_M_buffers);
        }

        assert(!buffers_to_write.empty());

        // 日志消息堆积, 超过后端的处理能力, 直接丢弃多余的buffer, 只保留2个
        if(buffers_to_write.size() > 25)
        {
            char buf[256];
            snprintf(buf, sizeof(buf), "Dropped log message at %s, %zd larger buffers.\n",
                     TimeStamp::now().to_string().c_str(), buffers_to_write.size() - 2);
            fputs(buf, stderr);
            file.append(buf, static_cast<int>(strlen(buf)));
            buffers_to_write.erase(buffers_to_write.begin() + 2, buffers_to_write.end());    
        }

        // 将buffers中的缓冲区写入磁盘
        for(const auto& buffer : buffers_to_write)
        {
            assert(buffer);
            file.append(buffer->data(), buffer->size());
        }

        if(buffers_to_write.size() > 2)
        {
            buffers_to_write.resize(2);
        }

        // 将buffers写入后, 空闲出来的buffer转移给buffer1/2
        if(!new_buffer1)
        {
            assert(!buffers_to_write.empty());
            new_buffer1 = std::move(buffers_to_write.back());
            buffers_to_write.pop_back();
            new_buffer1->reset();
        }

        if(!new_buffer2)
        {
            assert(!buffers_to_write.empty());
            new_buffer2 = std::move(buffers_to_write.back());
            buffers_to_write.pop_back();
            new_buffer2->reset();
        }

        buffers_to_write.clear();
        file.flush();
    }

    file.flush();
}

} // namespace mymuduo
