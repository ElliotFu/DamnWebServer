#include "count_down_latch.h"

Count_Down_Latch::Count_Down_Latch(int count)
    : mutex_(), condition_(mutex_), count_(count)
{
}

void Count_Down_Latch::wait()
{
    Mutex_Lock_Guard lock(mutex_);
    while (count_ > 0)
        condition_.wait();
}

void Count_Down_Latch::count_down()
{
    Mutex_Lock_Guard lock(mutex_);
    --count_;
    if (count_ == 0)
        condition_.notify_all();
}

int Count_Down_Latch::get_count() const
{
    Mutex_Lock_Guard lock(mutex_);
    return count_;
}