#include "event_loop.h"
#include "channel.h"
#include "poller.h"
#include "timer_queue.h"

#include <cassert>
#include <poll.h>
#include <sys/eventfd.h>
#include <unistd.h>

__thread Event_Loop* t_loop_in_this_thread = nullptr;
constexpr int k_poll_time_ms = 10000;

int create_eventfd()
{
    int eventfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (eventfd < 0) {
        // LOG_SYSERR << "Failed in eventfd";
        abort();
    }

    return eventfd;
}

Event_Loop::Event_Loop()
    : looping_(false),
      quit_(false),
      calling_pending_functors_(false),
      thread_id_(Current_Thread::tid()),
      poller_(Poller::new_default_poller(this)),
      timer_queue_(std::make_unique<Timer_Queue>(this)),
      wakeup_fd_(create_eventfd()),
      wakeup_channel_(std::make_unique<Channel>(this, wakeup_fd_))
{
    // LOG_TRACE << "EventLoop created " << this << " in thread " << thread_id_;
    if (t_loop_in_this_thread) {
        // LOG_FATAL << "Another EventLoop " << t_loopInThisThread
        //  << " exists in this thread " << threadId_;
    } else
        t_loop_in_this_thread = this;

    wakeup_channel_->set_read_callback(std::bind(&Event_Loop::handle_read, this));
    wakeup_channel_->enable_reading();
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
        active_channels_.clear();
        poll_return_time_ = poller_->poll(k_poll_time_ms, &active_channels_);
        for (Channel_List::iterator ite = active_channels_.begin(); ite != active_channels_.end(); ++ite) {
            (*ite)->handle_event(poll_return_time_);
        }
        do_pending_functors();
    }

    // LOG_TRACE << "EventLoop" << this << " stop looping";
    looping_ = false;
}

void Event_Loop::quit()
{
    quit_ = true;
    if (!is_in_loop_thread())
        wakeup();
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

void Event_Loop::remove_channel(Channel* channel)
{
    assert(channel->owner_loop() == this);
    assert_in_loop_thread();
    // if (eventHandling_) {
    //     assert(currentActiveChannel_ == channel ||
    //         std::find(activeChannels_.begin(), activeChannels_.end(), channel) == activeChannels_.end());
    // }
    poller_->remove_channel(channel);
}

void Event_Loop::run_in_loop(Functor cb)
{
    if (is_in_loop_thread())
        cb();
    else
        queue_in_loop(std::move(cb));
}

void Event_Loop::queue_in_loop(Functor cb)
{
    {
        Mutex_Lock_Guard lock(mutex_);
        pending_functions_.emplace_back(std::move(cb));
    }

    if (!is_in_loop_thread() || calling_pending_functors_)
        wakeup();
}

void Event_Loop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = ::write(wakeup_fd_, &one, sizeof one);
    if (n != sizeof one) {
        // LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
    }
}

void Event_Loop::handle_read()
{
    uint64_t one = 1;
    ssize_t n = ::read(wakeup_fd_, &one, sizeof one);
    if (n != sizeof one) {
        // LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
    }
}

void Event_Loop::do_pending_functors()
{
    std::vector<Functor> functors;
    calling_pending_functors_ = true;
    {
        Mutex_Lock_Guard lock(mutex_);
        functors.swap(pending_functions_);
    }

    for (size_t i = 0; i < functors.size(); ++i) {
        functors[i]();
    }

    calling_pending_functors_ = false;
}