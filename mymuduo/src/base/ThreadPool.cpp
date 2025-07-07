#include "mymuduo/base/ThreadPool.h"
#include "mymuduo/base/Logger.h"

using namespace mymuduo;

ThreadPool::ThreadPool(const std::string& type, size_t thread_num) 
                    : _thread_type(type), _stop(false)
{
    for(size_t i = 0; i < thread_num; i++) {
        _threads.emplace_back(
            [this]() {
                LOG_DEBUG("%s thread(%d) created.\n", _thread_type.c_str(), syscall(SYS_gettid));

                while(true) {
                    std::function<void()> task;

                    //////////////////////////////////
                    {
                        std::unique_lock<std::mutex> lock { _mutex };

                        // 等待生产者的条件变量, 满足条件时条件变量会唤醒进程, 否则会阻塞在这里
                        _condition.wait(lock, [this]{
                            // 只有当等待队列中有任务或者stop时才继续执行
                            return (_stop.load() || !_task_queue.empty());
                        });

                        // 只有下达stop指令并且没有任务时才能退出
                        if(_stop.load() && _task_queue.empty()) {
                            return;
                        }
                        
                        // 出队
                        task = std::move(_task_queue.front());
                        _task_queue.pop();
                    }
                    //////////////////////////////////

                    // 执行任务
                    task();
                }
            });
    }

    // 启动线程
    for (auto& t : _threads) {
        t.start();
    }
}

ThreadPool::~ThreadPool() {
    if (!_stop.load()) {
        LOG_WARN("ThreadPool(%x) is destroyed without calling stop().", this);
        this->stop();
    }
}

// 将任务添加到任务队列, 被条件变量唤醒 
void ThreadPool::push(std::function<void()> task) 
{
    {
        std::lock_guard<std::mutex> guard { _mutex };
        _task_queue.push(std::move(task));
    }

    _condition.notify_one();
}

void ThreadPool::stop() 
{
    if (_stop.load()) {
        return;
    }

    _stop = true;
    _condition.notify_all(); // 唤醒全部线程, 去执行剩余的任务 并且 退出

    // 等待所有线程的任务执行完毕后再退出
    for(auto& t : _threads) {
        if (t.joinable()) {
            t.join();
        }
    }
}
