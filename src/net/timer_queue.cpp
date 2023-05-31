#include "timer_queue.h"
#include "event_loop.h"
#include "timer_id.h"
#include <cassert>
#include <sys/timerfd.h>
#include <string.h>
#include <unistd.h>

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
      timers_(),
      calling_expired_timers_(false)
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
    assert(timers_.size() == active_timers_.size());
    std::vector<Entry> expired;
    Entry sentry(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
    Timer_List::iterator end = timers_.lower_bound(sentry);
    assert(end == timers_.end() || now < end->first);
    std::copy(timers_.begin(), end, back_inserter(expired));
    timers_.erase(timers_.begin(), end);

    for (const Entry& it : expired)
    {
        Active_Timer timer(it.second, it.second->sequence());
        size_t n = active_timers_.erase(timer);
        assert(n == 1); (void)n;
    }

    assert(timers_.size() == active_timers_.size());
    return expired;
}

bool Timer_Queue::insert(Timer* timer)
{
    loop_->assert_in_loop_thread();
    assert(timers_.size() == active_timers_.size());
    bool earliest = false;
    Timestamp when = timer->expiration();
    Timer_List::iterator ite = timers_.begin();
    if (ite == timers_.end() || when < ite->first)
        earliest = true;

    {
        auto result = timers_.emplace(when, timer);
        assert(result.second);
    }
    {
        auto result = active_timers_.emplace(timer, timer->sequence());
        assert(result.second);
    }
    assert(timers_.size() == active_timers_.size());
    return earliest;
}

void Timer_Queue::handle_read()
{
    loop_->assert_in_loop_thread();
    Timestamp now(Timestamp::now());
    read_timerfd(timerfd_, now);
    std::vector<Entry> expired = get_expired(now);
    calling_expired_timers_ = true;
    canceling_timers_.clear();
    for (const Entry& ite : expired)
        ite.second->run();

    calling_expired_timers_ = false;
    reset(expired, now);
}

void Timer_Queue::reset(const std::vector<Entry>& expired, Timestamp now)
{
    for (const Entry& it : expired) {
        Active_Timer timer(it.second, it.second->sequence());
        if (it.second->repeat() && canceling_timers_.find(timer) == canceling_timers_.end()) {
            it.second->restart(now);
            insert(it.second);
        }
        else {
            delete it.second; // FIXME: no delete please
        }
    }

    Timestamp next_expire;
    if (!timers_.empty())
        next_expire = timers_.begin()->second->expiration();

    if (next_expire.valid())
        reset_timerfd(timerfd_, next_expire);
}

void Timer_Queue::cancel(Timer_ID timer_id)
{
    loop_->run_in_loop(std::bind(&Timer_Queue::cancel_in_loop, this, timer_id));
}

void Timer_Queue::cancel_in_loop(Timer_ID timer_id)
{
    loop_->assert_in_loop_thread();
    assert(timers_.size() == active_timers_.size());
    Active_Timer timer(timer_id.timer_, timer_id.sequence_);
    Active_Timer_Set::iterator ite = active_timers_.find(timer);
    if (ite != active_timers_.end()) {
        size_t n = timers_.erase(Entry(ite->first->expiration(), ite->first));
        assert(n == 1);
        delete ite->first;
        active_timers_.erase(ite);
    } else if (calling_expired_timers_)
        canceling_timers_.insert(timer);

    assert(timers_.size() == active_timers_.size());
}