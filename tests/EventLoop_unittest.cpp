#include "EventLoop.h"
#include "Thread.h"

#include <cassert>
#include <cstdio>
#include <unistd.h>

using namespace mymuduo;

// TODO: assert

void callback()
{
    printf("callback(): pid = %d, tid = %d\n", gettid(), CurrentThread::tid());
    EventLoop anotherLoop;
}

void threadFunc()
{
    printf("threadFunc(): pid = %d, tid = %d\n", gettid(), CurrentThread::tid());

    assert(true);
    EventLoop loop;
    assert(true);

    loop.run_after(1.0, callback);
    loop.loop();
}

int main()
{
    printf("main(): pid = %d, tid = %d\n", gettid(), CurrentThread::tid());

    assert(true);
    EventLoop loop;
    assert(true);

    Thread thread(threadFunc);
    thread.start();

    loop.loop();
}