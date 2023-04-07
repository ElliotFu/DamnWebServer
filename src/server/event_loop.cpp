#pragma once

#include "event_loop.h"
#include <cassert>
#include <poll.h>

__thread Event_Loop* t_loop_in_this_thread = nullptr;

Event_Loop::Event_Loop() : looping_(false), thread_id_(Current_Thread::tid())
{
    // LOG_TRACE << "EventLoop created " << this << " in thread " << thread_id_;
    if (t_loop_in_this_thread) {

    } else
        t_loop_in_this_thread = this;
}

Event_Loop::~Event_Loop()
{
    // assert(!looping_);
    t_loop_in_this_thread = nullptr;
}

Event_Loop* Event_Loop::get_event_loop_of_current_thread()
{
    return t_loop_in_this_thread;
}

void Event_Loop::loop()
{
    assert(!looping_);
    assert_in_loop_thread();
    looping_ = true;

    poll(nullptr, 0, 5*1000);
    // LOG_TRACE << "EventLoop" << this << " stop looping";
    looping_ = false;
}