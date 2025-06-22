#include <sys/timerfd.h>

#include "TimerQueue.h"
#include "EventLoop.h"
#include "Logger.h"
#include "TimeStamp.h"

namespace mymuduo
{

namespace __detail
{
    /**
     * @brief 创建timerfd
     */
    int create_timerfd()
    {
        int tfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
        
        if(tfd < 0) {
            LOG_ERROR("%s:%s:%d timerfd create error:%d.\n", 
                __FILE__, __FUNCTION__, __LINE__, errno);
        }

        return tfd;
    }

    /**
     * @brief 计算when到now的时间, 填充为timespec结构体
     */
    struct timespec how_much_time_from_now(TimeStamp when)
    {
        ssize_t microseconds = (when.time_since_epoch() - 
                                TimeStamp::now().time_since_epoch()).count() / 1000;
        if(microseconds < 100) {
            microseconds = 100;
        }

        struct timespec ts;
        ts.tv_sec = static_cast<time_t>(microseconds / TimeStamp::kMicroSecondsPerSecond);
        ts.tv_nsec = static_cast<long>((microseconds % TimeStamp::kMicroSecondsPerSecond) * 1000);
        return ts;
    }

    /**
     * @brief
     */
    void read_timerfd(int timerfd, TimeStamp now)
    {
        ssize_t read_bytes;
        ssize_t nlen = ::read(timerfd, &read_bytes, sizeof(read_bytes));
        LOG_DEBUG("TimerQueue::handle_read() %ld.\n", read_bytes);
        if(nlen != sizeof(read_bytes)) {
            LOG_WARN("TimerQueue::handle_read() reads %ld nytes instead of 8.\n", nlen);
        }
    }

    /**
     * @brief 重置timerfd唤醒EventLoop线程的时间
     */
    void reset_timerfd(int timerfd, TimeStamp expiration)
    {
        struct itimerspec newVal;
        struct itimerspec oldVal;

        bzero(&newVal, sizeof(newVal));
        bzero(&oldVal, sizeof(oldVal));

        newVal.it_value = how_much_time_from_now(expiration);
        int ret = ::timerfd_settime(timerfd, 0, &newVal, &oldVal);
        if(ret) {
            LOG_WARN("timerfd_settime().\n");
        }
    }

} // namespace __detail

TimerQueue::TimerQueue(EventLoop *loop) :
            _M_loop(loop), 
            _M_timer_fd(__detail::create_timerfd()), 
            _M_timer_channel(loop, _M_timer_fd),
            _M_calling_expired_timers(false)
{
    _M_timer_channel.set_read_callback(std::bind(&TimerQueue::handle_read, this));
    _M_timer_channel.set_read_events();
}

TimerQueue::~TimerQueue()
{
    _M_timer_channel.unset_all_events();
    _M_timer_channel.remove();
    ::close(_M_timer_fd);
}


TimerId TimerQueue::add_timer(TimeStamp when, TimeDuration interval, TimerCallback func)
{
    Timer *timer = new Timer(when, interval, std::move(func));
    _M_loop->run_in_loop(std::bind(&TimerQueue::add_timer_in_loop, this, timer));
    return TimerId(timer->id());
}

void TimerQueue::cancel(TimerId timerId)
{
    _M_loop->run_in_loop(std::bind(&TimerQueue::cancel_in_loop, this, timerId));
}

void TimerQueue::add_timer_in_loop(Timer *timer)
{
    assert(_M_loop->is_loop_thread());

    // 将timer加入到TimerQueue中
    bool earliest_changed = insert(std::move(TimerPtr(timer)));

    // 如果此次timer的超时时间更近, 那么重新设置timerfd
    if(earliest_changed) {
        __detail::reset_timerfd(_M_timer_fd, timer->expiration());
    }
}

void TimerQueue::cancel_in_loop(TimerId timerId)
{
    /**
     * 不可以直接将TimerId中的Timer*进行dereference操作,
     * 因为TimerId不负责Timer的生命周期, 其中保存的Timer*可能失效
     * 所以必须查找
     */

    ActiveList::iterator it = _M_active_timers.find(timerId);

    if(it != _M_active_timers.end())
    {
        TimeStamp when = timerId.timer->expiration();
        auto itt = _M_timers.find(when);

        // 按时间戳排序, 有可能具有相同时间戳的定时器, 所以需重复查找(不过单位为ns, 所以额外开销不大)
        while(timerId.timer != itt->second.get()) {
            itt++;
        }

        // 一定能够找的到
        assert(itt != _M_timers.end() && timerId.timer == itt->second.get());

        _M_timers.erase(itt);
        _M_active_timers.erase(it);
    }

    // 若定时器已超时, 并且正在执行定时器任务, 则先将其加入CancelList中
    else if(_M_calling_expired_timers)
    {
        _M_cancel_timers.insert(timerId);
    }

    assert(_M_timers.size() == _M_active_timers.size());
}

void TimerQueue::handle_read()
{
    assert(_M_loop->is_loop_thread());

    TimeStamp now = TimeStamp::now();
    __detail::read_timerfd(_M_timer_fd, now);

    // 获取超时定时器
    TimerVec expired = get_expired(now);

    _M_calling_expired_timers = true;
    _M_cancel_timers.clear();

    // 执行定时器任务
    for(const TimerPtr& it : expired) {
        it->run();
    }

    _M_calling_expired_timers = false;

    // 重置超时定时器
    reset(expired, now);
}

TimerQueue::TimerVec TimerQueue::get_expired(TimeStamp now)
{
    assert(_M_timers.size() == _M_active_timers.size());
    
    TimerVec expired;

    // lower_bound返回第一个未到期的TimerMap的迭代器
    TimerMap::iterator end = _M_timers.lower_bound(now);
    assert(end == _M_timers.end() || now < end->first);
    
    // 将TimerMap中的定时器移动到expired中
    std::transform(
        _M_timers.begin(), end,
        std::back_inserter(expired),
        [](std::pair<const TimeStamp, std::unique_ptr<Timer>>& p) {
            return std::move(p.second);
        }
    );
    
    // 将定时器从TimerMap和_M_active_timers中删除 
    _M_timers.erase(_M_timers.begin(), end);
    for(const TimerPtr& it : expired) {
        std::size_t size = _M_active_timers.erase(it->id());
        assert(size == 1);
    }
    
    assert(_M_timers.size() == _M_active_timers.size());
    return expired;
}

void TimerQueue::reset(TimerVec &expired, TimeStamp now)
{
    for(TimerPtr& it : expired)
    {   
        // 若timer是重复定时器, 则重新启动并将其移动到TimerMap中
        if(it->repeat())
        {
            it->restart(now);
            insert(std::move(it));
        }
    }

    // 下一次超时时间
    TimeStamp nextExpire;

    if(!_M_timers.empty()) {
        nextExpire = _M_timers.begin()->first;
    }

    // 设置下次timerfd唤醒线程的时间
    if(nextExpire.valid()) {
        __detail::reset_timerfd(_M_timer_fd, nextExpire);
    }
}

bool TimerQueue::insert(TimerPtr timer)
{
    // 用于判断timer的超时时间是否更近
    bool earliest_changed = false;

    TimeStamp when = timer->expiration();
    TimerMap::iterator it = _M_timers.begin();

    // 若TimerMap为空 或者 timer到时时间更短, 标记为true
    if(it == _M_timers.end() || when < it->first) {
        earliest_changed = true;
    }
    
    {
        // 将timer同时添加到ActiveList
        std::pair<ActiveList::iterator, bool> result = _M_active_timers.insert(timer->id());
        assert(result.second);
    }

    {
        // 将timer交由unique_ptr管理, 并添加到TimerMap
        TimerMap::iterator result = _M_timers.insert({when, std::move(timer)});
        assert(result != _M_timers.end());
    }    

    assert(_M_timers.size() == _M_active_timers.size());
    return earliest_changed;
}

} // namespace mymuduo