#include "Thread.h"
#include "CurrentThread.h"

std::atomic<int> Thread::_M_num_created = 0;

Thread::Thread(ThreadFunc func, const std::string &name) :
            _M_started(false), _M_joined(false), 
            _M_tid(0), _M_name(name),
            _M_func(std::move(func))
{
    set_default_name();
}

Thread::~Thread() {
    if(_M_started && !_M_joined) {
        _M_thread->detach();
    }    
}

void Thread::start() {
    _M_started = true;

    sem_t sem;
    sem_init(&sem, 0, 0);

    _M_thread = std::shared_ptr<std::thread>(new std::thread([&](){
        _M_tid = CurrentThread::tid();
        sem_post(&sem);
        _M_func();
    }));

    // 等待子线程的tid值
    sem_wait(&sem);
}

void Thread::join() {
    _M_joined = true;
    _M_thread->join();
}

void Thread::set_default_name() {
    int num = ++_M_num_created;
    if(_M_name.empty()) {
        char buf[32] = {0};
        snprintf(buf, sizeof(buf), "Thread%d", num);
        _M_name = buf;
    }
}
