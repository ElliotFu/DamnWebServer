#pragma once

#include "base/noncopyable.h"
#include "base/timestamp.h"
#include <atomic>
#include <functional>

class Timer : Noncopyable
{
public:
    typedef std::function<void()> Timer_Callback;

    Timer(Timer_Callback cb, Timestamp when, double interval)
        : callback_(std::move(cb)),
          expiration_(when),
          interval_(interval),
          repeat_(interval > 0.0),
          sequence_(num_created_.fetch_add(1, std::memory_order_relaxed))
    {}

    void run() const { callback_(); }

    Timestamp expiration() const { return expiration_; }
    bool repeat() const { return repeat_; }
    int64_t sequence() const { return sequence_; }

    void restart(Timestamp now);

    static int64_t num_created() { return num_created_.load(); }

private:
    const Timer_Callback callback_;
    Timestamp expiration_;
    const double interval_;
    const bool repeat_;
    const int64_t sequence_;

    static std::atomic_int64_t num_created_;
};
