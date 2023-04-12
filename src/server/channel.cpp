#include "channel.h"
#include "event_loop.h"
#include <poll.h>
#include <sstream>

const int Channel::k_none_event = 0;
const int Channel::k_read_event = POLLIN | POLLPRI;
const int Channel::k_write_event = POLLOUT;

Channel::Channel(Event_Loop* loop, int fd_arg)
    : loop_(loop), fd_(fd_arg), events_(0), revents_(0), index_(-1)
{
}

Channel::~Channel()
{
    // if (loop_->is_in_loop_thread()) {
    //     assert(!loop_->has_channel(this));
    // }
}

void Channel::update()
{
    loop_->update_channel(this);
}

void Channel::handle_event()
{
    if (revents_ & POLLNVAL) {
        // LOG_WARN << "Channel::handle_event() POLLNVAL";
    }

    if (revents_ & (POLLERR | POLLNVAL)) {
        if (error_callback_)
            error_callback_();
    }

    if (revents_ & (POLLIN | POLLPRI | POLLRDHUP)) {
        if (read_callback_)
            read_callback_();
    }

    if (revents_ & POLLOUT) {
        if (write_callback_)
            write_callback_();
    }
}

string Channel::revents_to_string() const
{
    return events_to_string(fd_, revents_);
}

string Channel::events_to_string() const
{
    return events_to_string(fd_, events_);
}

string Channel::events_to_string(int fd, int events)
{
    std::ostringstream oss;
    oss << fd << ": ";
    if (events & POLLIN)
        oss << "IN ";
    if (events & POLLPRI)
        oss << "PRI ";
    if (events & POLLOUT)
        oss << "OUT ";
    if (events & POLLHUP)
        oss << "HUP ";
    if (events & POLLRDHUP)
        oss << "RDHUP ";
    if (events & POLLERR)
        oss << "ERR ";
    if (events & POLLNVAL)
        oss << "NVAL ";

    return oss.str();
}