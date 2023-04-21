#pragma once

#include "timer.h"
#include "timer_id.h"
#include "base/current_thread.h"
#include "base/noncopyable.h"
#include "base/timestamp.h"
#include "base/mutex.h"
#include <atomic>
#include <cassert>
#include <memory>
#include <mutex>
#include <vector>
#include <sys/types.h>

class Channel;
class Timer_Queue;
class Poller;

class Event_Loop : Noncopyable
{
public:
    typedef std::function<void()> Functor;

    Event_Loop();
    ~Event_Loop();

    void loop();
    void quit();

    void assert_in_loop_thread() { assert(is_in_loop_thread()); }

    bool is_in_loop_thread() const { return thread_id_ == Current_Thread::tid(); }
    static Event_Loop* get_event_loop_of_current_thread();

    void update_channel(Channel* channel);
    bool has_channel(Channel* channel);
    void remove_channel(Channel* channel);

    Timer_ID run_at(const Timestamp& time, const  Timer::Timer_Callback& cb);
    Timer_ID run_after(double delay, const Timer::Timer_Callback& cb);
    Timer_ID run_every(double interval, const Timer::Timer_Callback& cb);

    void run_in_loop(Functor cb);
    void queue_in_loop(Functor cb);
    void wakeup();
private:
    typedef std::vector<Channel*> Channel_List;

    void handle_read();
    void do_pending_functors();

    bool looping_;
    std::atomic<bool> quit_;
    bool calling_pending_functors_;
    const pid_t thread_id_;
    Timestamp poll_return_time_;
    std::unique_ptr<Poller> poller_;
    Channel_List active_channels_;
    std::unique_ptr<Timer_Queue> timer_queue_;
    int wakeup_fd_;
    std::unique_ptr<Channel> wakeup_channel_;
    mutable Mutex mutex_;
    std::vector<Functor> pending_functions_ GUARDED_BY(mutex_);
};