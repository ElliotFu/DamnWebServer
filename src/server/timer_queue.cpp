#include "timer_queue.h"
#include "event_loop.h"
#include "timer_id.h"
#include <cassert>
#include <sys/timerfd.h>
#include <string.h>

int create_timerfd()
{
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (timerfd < 0) {
        // LOG_SYSFATAL << "Failed in timerfd_create";
    }
    return timerfd;
}

timespec how_much_time_from_now(Timestamp when)
{
    int64_t microseconds = when.microseconds_since_epoch() - Timestamp::now().microseconds_since_epoch();
    if (microseconds < 100)
        microseconds = 100;

    timespec ts;
    ts.tv_sec = static_cast<time_t>(microseconds / Timestamp::k_microseconds_per_second);
    ts.tv_nsec = static_cast<long>((microseconds % Timestamp::k_microseconds_per_second) * 1000);
    return ts;
}

void read_timerfd(int timerfd, Timestamp now)
{
    uint64_t howmany;
    ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
    // LOG_TRACE << "TimerQueue::handleRead() " << howmany << " at " << now.toString();
    if (n != sizeof howmany) {
        // LOG_ERROR << "TimerQueue::handleRead() reads " << n << " bytes instead of 8";
    }
}

void reset_timerfd(int timerfd, Timestamp expiration)
{
    // wake up loop by timerfd_settime()
    itimerspec new_value;
    itimerspec old_value;
    memset(&new_value, 0, sizeof new_value);
    memset(&old_value, 0, sizeof old_value);
    new_value.it_value = how_much_time_from_now(expiration);
    int ret = ::timerfd_settime(timerfd, 0, &new_value, &old_value);
    if (ret) {
        // LOG_SYSERR << "timerfd_settime()";
    }
}

Timer_Queue::Timer_Queue(Event_Loop* loop)
    : loop_(loop),
      timerfd_(create_timerfd()),
      timerfd_channel_(loop, timerfd_),
      timers_()
{
    timerfd_channel_.set_read_callback(std::bind(&Timer_Queue::handle_read, this));
    timerfd_channel_.enable_reading();
}

Timer_Queue::~Timer_Queue()
{
    timerfd_channel_.disable_all();
    timerfd_channel_.remove();
    ::close(timerfd_);
    // unique_ptr helps destruct Timer in timers_
}

Timer_ID Timer_Queue::add_timer(const Timer::Timer_Callback& cb, Timestamp when, double interval)
{
    Timer* timer = new Timer(std::move(cb), when, interval);
    loop_->run_in_loop(std::bind(&Timer_Queue::add_timer_in_loop, this, timer));
    return Timer_ID(timer, timer->sequence());
}

void Timer_Queue::add_timer_in_loop(Timer* timer)
{
    loop_->assert_in_loop_thread();
    bool earliest = insert(timer);
    if (earliest)
        reset_timerfd(timerfd_, timer->expiration());
}

std::vector<Timer_Queue::Entry> Timer_Queue::get_expired(Timestamp now)
{
    std::vector<Entry> expired;
    Entry border = std::make_pair(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
    Timer_List::iterator ite = timers_.lower_bound(border);
    assert(ite == timers_.end() || now < ite->first);
    std::copy(timers_.begin(), ite, back_inserter(expired));
    timers_.erase(timers_.begin(), ite);
    return expired;
}

bool Timer_Queue::insert(Timer* timer)
{
    loop_->assert_in_loop_thread();
    bool earliest = false;
    Timestamp when = timer->expiration();
    Timer_List::iterator ite = timers_.begin();
    if (ite == timers_.end() || when < ite->first)
        earliest = true;

    {
        auto result = timers_.emplace(when, timer);
        assert(result.second);
    }
    return earliest;
}

void Timer_Queue::handle_read()
{
    loop_->assert_in_loop_thread();
    Timestamp now(Timestamp::now());
    read_timerfd(timerfd_, now);
    std::vector<Entry> expired = get_expired(now);
    for (const Entry& ite : expired)
        ite.second->run();

    reset(expired, now);
}

void Timer_Queue::reset(const std::vector<Entry>& expired, Timestamp now)
{

}