#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"
#include "EventLoop.h"
#include "Thread.h"

#include <cstdio>
#include <unistd.h>

using namespace mymuduo;

void print(EventLoop *p = nullptr)
{
    printf("print(): pid = %d, tid = %d, loop = %p\n",
            getpid(), CurrentThread::tid(), p);
}

void init(EventLoop *p)
{
    printf("init(): pid = %d, tid = %d, loop = %p\n",
            gettid(), CurrentThread::tid(), p);
}

int main()
{
    print();

    EventLoop loop;
    loop.run_after(11, std::bind(&EventLoop::quit, &loop));

    {
        printf("Single thread %p:\n", &loop);
        EventLoopThreadPool model(&loop, "single");
        model.set_thread_num(0);
        model.start(init);
        assert(model.get_next_loop() == &loop);
        assert(model.get_next_loop() == &loop);
        assert(model.get_next_loop() == &loop);
    }

    {
        printf("Another thread:\n");
        EventLoopThreadPool model(&loop, "another");
        model.set_thread_num(1);
        model.start(init);

        EventLoop *nextLoop = model.get_next_loop();
        nextLoop->run_after(2, std::bind(print, nextLoop));
        assert(nextLoop != &loop);
        assert(nextLoop == model.get_next_loop());
        assert(nextLoop == model.get_next_loop());
        ::sleep(3);
    }

    {
        printf("Three threads:\n");
        EventLoopThreadPool model(&loop, "three");
        model.set_thread_num(3);
        model.start(init);

        EventLoop *nextLoop = model.get_next_loop();
        nextLoop->run_in_loop(std::bind(print, nextLoop));
        assert(nextLoop != &loop);
        assert(nextLoop != model.get_next_loop());
        assert(nextLoop != model.get_next_loop());
        assert(nextLoop == model.get_next_loop());
    }

    printf("Here!\n");
    loop.loop();
}