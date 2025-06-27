#include "base/ThreadPool.h"
#include "base/Logger.h"

using namespace mymuduo;

ThreadPool::ThreadPool(const std::string& type, size_t thread_num) 
                    : _M_thread_type(type), _M_stop(false)
{
    for(size_t i = 0; i < thread_num; i++) {
        _M_threads.emplace_back(
            [this]() {
                LOG_DEBUG("%s thread(%d) created.\n", _M_thread_type.c_str(), syscall(SYS_gettid));

                while(true) {
                    std::function<void()> task;

                    //////////////////////////////////
                    {
                        std::unique_lock<std::mutex> lock { _M_mutex };

                        // 等待生产者的条件变量, 满足条件时条件变量会唤醒进程, 否则会阻塞在这里
                        _M_condition.wait(lock, [this]{
                            // 只有当等待队列中有任务或者stop时才继续执行
                            return (_M_stop.load() || !_M_task_queue.empty());
                        });

                        // 只有下达stop指令并且没有任务时才能退出
                        if(_M_stop.load() && _M_task_queue.empty()) {
                            return;
                        }
                        
                        // 出队
                        task = std::move(_M_task_queue.front());
                        _M_task_queue.pop();
                    }
                    //////////////////////////////////

                    // 执行任务
                    task();
                }
            });
    }

    // 启动线程
    for (auto& t : _M_threads) {
        t.start();
    }
}

ThreadPool::~ThreadPool() {
    if (!_M_stop.load()) {
        LOG_WARN("ThreadPool(%x) is destroyed without calling stop().", this);
        this->stop();
    }
}

// 将任务添加到任务队列, 被条件变量唤醒 
void ThreadPool::push(std::function<void()> task) 
{
    {
        std::lock_guard<std::mutex> guard { _M_mutex };
        _M_task_queue.push(std::move(task));
    }

    _M_condition.notify_one();
}

void ThreadPool::stop() 
{
    if (_M_stop.load()) {
        return;
    }

    _M_stop = true;
    _M_condition.notify_all(); // 唤醒全部线程, 去执行剩余的任务 并且 退出

    // 等待所有线程的任务执行完毕后再退出
    for(auto& t : _M_threads) {
        if (t.joinable()) {
            t.join();
        }
    }
}
