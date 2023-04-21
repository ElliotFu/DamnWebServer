#include "epoll_poller.h"
#include "base/timestamp.h"
#include "channel.h"
#include "timer_queue.h"
#include <error.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>

constexpr int k_new = -1;
constexpr int k_added = 1;
constexpr int k_deleted = 2;

Epoll_Poller::Epoll_Poller(Event_Loop* loop)
    : Poller(loop),
      epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
      events_(k_init_event_list_size)
{
    if (epollfd_ < 0) {
        // LOG_SYSFATAL << "Epoll_Poller::Epoll_Poller";
    }
}

Epoll_Poller::~Epoll_Poller()
{
    ::close(epollfd_);
}

Timestamp Epoll_Poller::poll(int timeout_ms, Channel_List* active_channels)
{
    int num_events = ::epoll_wait(epollfd_,
                                  events_.data(),
                                  static_cast<int>(events_.size()),
                                  timeout_ms);

    int saved_errno = errno;
    Timestamp now(Timestamp::now());
    if (num_events > 0)
    {
        // LOG_TRACE << num_events << " events happened";
        fill_active_channels(num_events, active_channels);
        if (static_cast<size_t>(num_events) == events_.size())
            events_.resize(events_.size() * 2);
    } else if (num_events == 0) {
        // LOG_TRACE << "nothing happened";
    }
    else {
        if (saved_errno != EINTR) {
            errno = saved_errno;
            // LOG_SYSERR << "EPollPoller::poll()"
        }
    }

    return now;
}

void Epoll_Poller::fill_active_channels(int num_events, Channel_List* active_channels) const
{
    assert(static_cast<size_t>(num_events) <= events_.size());
    for (int i = 0; i < num_events; ++i) {
        // 为什么 channel* 和 epoll_data* 能够相互转化
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        active_channels->emplace_back(channel);
    }
}

void Epoll_Poller::update_channel(Channel* channel)
{
    Poller::assert_in_loop_thread();
    const int idx = channel->index();
    // LOG_TRACE << "fd = " << channel->fd()
    //      << " events = " << channel->events() << " index = " << index;
    if (idx == k_new || idx == k_deleted) {
        // a new one, add with EPOLL_CTL_ADD
        int fd = channel->fd();
        if (idx == k_new) {
            assert(channels_.find(fd) == channels_.end());
            channels_[fd] = channel;
        } else {
            assert(channels_.find(fd) != channels_.end());
            assert(channels_[fd] == channel);
        }

        channel->set_index(k_added);
        update(EPOLL_CTL_ADD, channel);
    } else {
        // update existing one with EPOLL_CTL_MOD/DEL
        int fd = channel->fd();
        assert(channels_.find(fd) != channels_.end());
        assert(channels_[fd] == channel);
        assert(idx == k_added);
        if (channel->is_none_event()) {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(k_deleted);
        } else {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void Epoll_Poller::remove_channel(Channel* channel)
{
    Poller::assert_in_loop_thread();
    int fd = channel->fd();
    // LOG_TRACE << "fd = " << fd;
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);
    assert(channel->is_none_event());
    int idx = channel->index();
    assert(idx == k_added || idx == k_deleted);
    size_t n = channels_.erase(fd);
    assert(n == 1);
    if (idx == k_added)
        update(EPOLL_CTL_DEL, channel);

    channel->set_index(k_new);
}

void Epoll_Poller::update(int operation, Channel* channel)
{
    epoll_event event;
    memset(&event, 0, sizeof event);
    event.events = channel->events();
    event.data.ptr = channel;
    int fd = channel->fd();
    // LOG_TRACE << "epoll_ctl op = " << operation_to_string(operation)
    //    << " fd = " << fd << " event = {" << channel->events_to_string() << " }";
    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0) {
        if (operation == EPOLL_CTL_DEL) {
            // LOG_SYSERR << "epoll_ctl op =" << operation_to_string(operation) << " fd =" << fd;
        } else {
            // LOG_SYSFATAL << "epoll_ctl op =" << operation_to_string(operation) << " fd =" << fd;
        }
    }
}

const char* Epoll_Poller::operation_to_string(int op)
{
    switch (op) {
    case EPOLL_CTL_ADD:
        return "ADD";
    case EPOLL_CTL_DEL:
        return "DEL";
    case EPOLL_CTL_MOD:
        return "MOD";
    default:
        assert(false);
        return "Unknown Operation";
    }
}

Timer_ID Event_Loop::run_at(const Timestamp& time, const Timer::Timer_Callback& cb)
{
    return timer_queue_->add_timer(cb, time, 0.0);
}

Timer_ID Event_Loop::run_after(double delay, const Timer::Timer_Callback& cb)
{
    Timestamp time(add_time(Timestamp::now(), delay));
    return run_at(time, cb);
}

Timer_ID Event_Loop::run_every(double interval, const Timer::Timer_Callback& cb)
{
    Timestamp time(add_time(Timestamp::now(), interval));
    return timer_queue_->add_timer(cb, time, interval);
}