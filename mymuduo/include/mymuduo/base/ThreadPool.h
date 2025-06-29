/**
 * 
 * ThreadPool头文件
 * 
 */
#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "mymuduo/base/Thread.h"

#include <vector>
#include <string>
#include <queue>
#include <sys/syscall.h>
#include <mutex>
#include <unistd.h>
#include <condition_variable>
#include <functional>
#include <atomic>

namespace mymuduo {

class ThreadPool
{
public:

    ThreadPool(const std::string& type, size_t thread_num);

    ~ThreadPool();

    void stop();

    void push(std::function<void()> task);
    
    const size_t size() const noexcept { return _M_threads.size(); }

private:
    // 线程池中的线程
    std::vector<Thread> _M_threads;

    // 任务队列
    std::queue<std::function<void()>> _M_task_queue;
    // 任务队列同步的互斥锁
    std::mutex _M_mutex;

    // 任务队列同步的条件变量
    std::condition_variable _M_condition;

    // 在析构函数中, 将其值设置为ture, 全部的线程将退出
    std::atomic<bool> _M_stop;

    // 线程种类
    std::string _M_thread_type; // 取值为IO, WORK
};

} // namespace mymuduo

#endif // THREADPOOL_H