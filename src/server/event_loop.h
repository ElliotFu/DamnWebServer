#pragma once

#include "base/current_thread.h"
#include "base/noncopyable.h"
#include <cassert>
#include <memory>
#include <vector>
#include <sys/types.h>

class Channel;
class Poller;

class Event_Loop : Noncopyable
{
public:
    Event_Loop();
    ~Event_Loop();

    void loop();
    void quit();

    void assert_in_loop_thread() { assert(is_in_loop_thread()); }

    bool is_in_loop_thread() const { return thread_id_ == Current_Thread::tid(); }
    static Event_Loop* get_event_loop_of_current_thread();

    void update_channel(Channel* channel);
    bool has_channel(Channel* channel);

private:
    typedef std::vector<Channel*> Channel_List;

    bool looping_;  // atomic
    bool quit_;     // atomic
    const pid_t thread_id_;
    std::shared_ptr<Poller> poller_;
    Channel_List active_channels_;
};