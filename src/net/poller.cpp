#include "poller.h"
#include "channel.h"
#include "epoll_poller.h"

Poller::Poller(Event_Loop* loop) : owner_loop_(loop)
{
}

Poller::~Poller() = default;

bool Poller::has_channel(Channel* channel) const
{
    assert_in_loop_thread();
    Channel_Map::const_iterator ite = channels_.find(channel->fd());
    return ite != channels_.end() && ite->second == channel;
}

Poller* Poller::new_default_poller(Event_Loop* loop)
{
    return new Epoll_Poller(loop);
}