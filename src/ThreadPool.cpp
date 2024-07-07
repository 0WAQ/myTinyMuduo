#include "../include/ThreadPool.hpp"

ThreadPool::ThreadPool(const std::string& type, size_t thread_num) 
                    : _M_thread_type(type), _M_stop(false)
{
    // 启动thread_num个线程, 将每个线程阻塞在条件变量上
    for(size_t i = 0; i < thread_num; i++)
    {   
        // 利用lambda函数创建线程, 并添加到线程池中
        _M_threads.emplace_back(
            [this]() {
                // 打印进程ID
                printf("%s thread(%d) created.\n", this->_M_thread_type.c_str(), syscall(SYS_gettid));

                while(_M_stop == false) {
                    std::function<void()> task;

                    //////////////////////////////////
                    {   // 锁作用域开始
                        std::unique_lock<std::mutex> lock(this->_M_mutex);

                        // 等待生产者的条件变量, 满足条件时条件变量会唤醒进程, 否则会阻塞在这里
                        this->_M_condition.wait(lock, [this]{
                            // 只有当等待队列中有任务或者stop时才继续执行
                            return (this->_M_stop || !this->_M_task_queue.empty());
                        });

                        // 只有下达stop指令并且没有任务时才能退出
                        if(this->_M_stop && this->_M_task_queue.empty())
                            return;
                        
                        // 出队
                        task = std::move(this->_M_task_queue.front()); // 移动语义, 避免拷贝
                        this->_M_task_queue.pop();

                    }   // 锁作用域结束
                    //////////////////////////////////

                    // 执行任务
                    task();
                }
            });
    }
}

void ThreadPool::push(std::function<void()> task) 
{
    ////////////////////////////////////////
    {   // 锁所用域开始

        std::lock_guard<std::mutex> lock(_M_mutex);
        _M_task_queue.push(task);
    
    }   // 锁作用域结束
    ////////////////////////////////////////

    // 唤醒一个线程
    _M_condition.notify_one();
}

ThreadPool::~ThreadPool() 
{
    _M_stop = true;
    _M_condition.notify_all(); // 唤醒全部线程, 去执行剩余的任务 并且 退出

    // 等待所有线程执行完毕后, 任务退出
    for(auto &th : _M_threads) {
        th.join();
    }
}
