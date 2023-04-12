#include "event_loop.h"
#include "channel.h"
#include "poller.h"
#include <cassert>
#include <poll.h>

__thread Event_Loop* t_loop_in_this_thread = nullptr;
constexpr int k_poll_time_ms = 10000;

Event_Loop::Event_Loop()
    : looping_(false),
      quit_(false),
      thread_id_(Current_Thread::tid()),
      poller_(Poller::new_default_poller(this))
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
    quit_ = false;
    while (!quit_) {
        active_channels_ = poller_->poll(k_poll_time_ms);
        for (Channel_List::iterator ite = active_channels_.begin(); ite != active_channels_.end(); ++ite) {
            (*ite)->handle_event();
        }
    }

    // LOG_TRACE << "EventLoop" << this << " stop looping";
    looping_ = false;
}

void Event_Loop::quit()
{
    quit_ = true;
    // wakeup();
}

void Event_Loop::update_channel(Channel* channel)
{
    assert(channel->owner_loop() == this);
    assert_in_loop_thread();
    poller_->update_channel(channel);
}

bool Event_Loop::has_channel(Channel* channel)
{
    assert(channel->owner_loop() == this);
    assert_in_loop_thread();
    return poller_->has_channel(channel);
}