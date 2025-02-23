/**
 * 
 * Poller的公共源文件, 可以添加依赖关系
 * 
 */
#include <stdlib.h>
#include "Poller.h"
#include "EPollPoller.h"

Poller* Poller::new_default_poller(EventLoop *loop)
{
    if(::getenv("MUDUO_USE_POLL")) {
        return nullptr;     // 生成Poll实例
    }
    else {
        return new EPollPoller(loop);  // 生成Epoll实例
    }
}