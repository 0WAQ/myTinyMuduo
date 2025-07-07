
#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

#include "mymuduo/base/Thread.h"

namespace mymuduo {

class ThreadPool {
public:
    ThreadPool(const std::string& type, size_t thread_num);
    ~ThreadPool();

    void stop();

    void push(std::function<void()> task);

    const size_t size() const noexcept { return _threads.size(); }

private:
    // 线程池中的线程
    std::vector<Thread> _threads;

    // 任务队列
    std::queue<std::function<void()>> _task_queue;
    // 任务队列同步的互斥锁
    std::mutex _mutex;

    // 任务队列同步的条件变量
    std::condition_variable _condition;

    // 在析构函数中, 将其值设置为ture, 全部的线程将退出
    std::atomic<bool> _stop;

    // 线程种类
    std::string _thread_type; // 取值为IO, WORK
};

} // namespace mymuduo

#endif // THREADPOOL_H