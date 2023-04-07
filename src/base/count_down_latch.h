#pragma once

#include "condition.h"
#include "mutex.h"

class Count_Down_Latch : Noncopyable
{
public:
    explicit Count_Down_Latch(int count);

    void wait();
    void count_down();
    int get_count() const;

private:
    mutable Mutex mutex_;
    Condition condition_ GUARDED_BY(mutex_);
    int count_ GUARDED_BY(mutex_);
};