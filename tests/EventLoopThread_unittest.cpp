#include "EventLoopThread.h"
#include "EventLoop.h"
#include "Thread.h"

#include <cstdio>
#include <unistd.h>

using namespace mymuduo;

void print(EventLoop *p = nullptr)
{
    printf("print: pid = %d, tid = %d, loop = %p\n",
            getpid(), CurrentThread::tid(), p);
}

void quit(EventLoop *p)
{
    print(p);
    p->quit();
}

int main()
{
    print();

    {
        EventLoopThread th1;
    }

    {
        EventLoopThread th2;

        EventLoop *loop = th2.start_loop();
        loop->run_in_loop(std::bind(print, loop));
        CurrentThread::sleep_usec(500 * 1000);
    }

    {
        EventLoopThread th3;

        EventLoop *loop = th3.start_loop();
        loop->run_in_loop(std::bind(print, loop));
        CurrentThread::sleep_usec(500 * 1000);
    }
}