#pragma once

#include "poller.h"
#include <string>

struct epoll_event;

class Epoll_Poller : public Poller
{
public:
    Epoll_Poller(Event_Loop* loop);
    ~Epoll_Poller() override;

    Timestamp poll(int timeout_ms, Channel_List* active_channels) override;
    void update_channel(Channel* channel) override;
    void remove_channel(Channel* channel) override;

private:
    static const int k_init_event_list_size = 16;
    static const char* operation_to_string(int op);

    void fill_active_channels(int num_events, Channel_List* active_channels) const;
    void update(int operation, Channel* channel);

    typedef std::vector<epoll_event> Event_List;

    int epollfd_;
    Event_List events_;
};