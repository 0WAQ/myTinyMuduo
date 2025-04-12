#include "EventLoop.h"
#include "EventLoopThread.h"
#include "Thread.h"

#include <cstdio>
#include <unistd.h>

using namespace mymuduo;

int cnt = 0;
EventLoop *gloop;

void print_tid()
{
    printf("pid = %d, tid = %d\n", gettid(), CurrentThread::tid());
    printf("now %s\n", TimeStamp::now().to_string().c_str());
}

void print(const char *msg)
{
    printf("msg %s %s\n", TimeStamp::now().to_string().c_str(), msg);
    if(++cnt == 20) {
        gloop->quit();
    }
}

void cancel(TimerId timer)
{
    gloop->cancel(timer);
    printf("cancelled at %s\n", TimeStamp::now().to_string().c_str());
}

int main()
{
    print_tid();
    ::sleep(1);

    {
        EventLoop loop;
        gloop = &loop;
        
        print("main");
        loop.run_after(1, std::bind(print, "once 1"));
        loop.run_after(1.5, std::bind(print, "once 1.5"));
        loop.run_after(2.5, std::bind(print, "once 2.5"));
        loop.run_after(3.5, std::bind(print, "once 3.5"));

        TimerId t45 = loop.run_after(4.5, std::bind(print, "once 4.5"));
        loop.run_after(4.2, std::bind(cancel, t45));
        loop.run_after(4.8, std::bind(cancel, t45));
        loop.run_every(2, std::bind(print, "every 2"));

        TimerId t3 = loop.run_every(3, std::bind(print, "every 3"));
        loop.run_after(1, std::bind(print, "once 1"));

        loop.loop();
        print("main loop exits");
    }

    ::sleep(1);

    {
        EventLoopThread thread{ [](EventLoop* p) {
        }, "TimerQueue_unittest-thread"};
        EventLoop *loop = thread.start_loop();
        loop->run_after(2, print_tid);
        ::sleep(3);
        print("thread loop exits");
    }
}