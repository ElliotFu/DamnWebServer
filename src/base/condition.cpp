#include "condition.h"
#include <cerrno>

bool Condition::wait_for_seconds(double seconds)
{
    timespec abstime;
    clock_gettime(CLOCK_REALTIME, &abstime);
    const __int64_t k_nano_seconds_per_second = 1000000000;
    __int64_t nanoseconds = static_cast<__int64_t>(seconds * k_nano_seconds_per_second);
    abstime.tv_sec += static_cast<time_t>((abstime.tv_nsec + nanoseconds) / k_nano_seconds_per_second);
    abstime.tv_nsec = static_cast<long>((abstime.tv_nsec + nanoseconds) % k_nano_seconds_per_second);
    Mutex::Unassign_Guard ug(mutex_);
    return ETIMEDOUT == pthread_cond_timedwait(&pcond_, mutex_.get_pthread_mutex(), &abstime);
}