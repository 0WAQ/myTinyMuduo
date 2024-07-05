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

    /**
     * 
     * @describe: 用来初始化线程
     * @param:    size_t
     * 
     */
    ThreadPool(const std::string& type, size_t thread_num);


    /**
     * 
     * @describe: 将任务添加到任务队列中
     * @param:    std::function<void()>
     * @return:   void
     * 
     */
    void push(std::function<void()> task);


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