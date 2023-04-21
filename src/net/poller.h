#pragma once

#include "event_loop.h"
#include "base/noncopyable.h"
#include <vector>
#include <map>

class Channel;
class Timestamp;

class Poller : Noncopyable
{
public:
    typedef std::vector<Channel*> Channel_List;

    Poller(Event_Loop* loop);
    virtual ~Poller();

    virtual Timestamp poll(int timeout_ms, Channel_List* time) = 0;
    virtual void update_channel(Channel* channel) = 0;
    virtual void remove_channel(Channel* channel) = 0;

    virtual bool has_channel(Channel* channel) const;
    void assert_in_loop_thread() const { owner_loop_->assert_in_loop_thread(); }

    static Poller* new_default_poller(Event_Loop* loop);

protected:
    typedef std::map<int, Channel*> Channel_Map;
    Channel_Map channels_;

private:
    Event_Loop* owner_loop_;
};