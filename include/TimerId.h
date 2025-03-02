#ifndef TIMERID_H
#define TIMERID_H

class Timer;

/**
 * @brief
 */
class TimerId
{
public:

    TimerId() : _M_timer(nullptr), _M_id(0) { }

private:

    // 所属定时器
    Timer* _M_timer;

    // 定时器id
    long _M_id;
};

#endif // TIMERID_H