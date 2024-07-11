#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <queue>
#include <sys/syscall.h>
#include <mutex>
#include <unistd.h>
#include <thread>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>

/**
 * 
 * 线程池类
 * 
 */
class ThreadPool
{
public:

    /// @brief 初始化线程池
    /// @param type 线程类型
    /// @param thread_num 线程数量
    ThreadPool(const std::string& type, size_t thread_num);


    /// @brief 将任务添加到任务队列
    /// @param task 函数对象
    void push(std::function<void()> task);


    /// @brief 获取线程池中的线程数量
    /// @return 
    std::size_t size();


    /// @brief 终止线程池
    void stop();
    

    ~ThreadPool();


private:
    // 线程池中的线程
    std::vector<std::thread> _M_threads;

    // 任务队列
    std::queue<std::function<void()>> _M_task_queue;
    // 任务队列同步的互斥锁
    std::mutex _M_mutex;

    // 任务队列同步的条件变量
    std::condition_variable _M_condition;

    // 在析构函数中, 将其值设置为ture, 全部的线程将退出
    std::atomic_bool _M_stop;

    // 线程种类
    std::string _M_thread_type; // 取值为IO, WORK
};