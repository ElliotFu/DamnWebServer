#pragma once

#include "base/current_thread.h"
#include "base/noncopyable.h"
#include <cassert>
#include <sys/types.h>

class Event_Loop : Noncopyable
{
public:
    Event_Loop();
    ~Event_Loop();

    void loop();

    void assert_in_loop_thread() { assert(is_in_loop_thread()); }

    bool is_in_loop_thread() const { return thread_id_ == Current_Thread::tid(); }
    static Event_Loop* Event_Loop::get_event_loop_of_current_thread();

private:
    bool looping_; // atomic
    const pid_t thread_id_;
};