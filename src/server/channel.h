#pragma once

#include "base/noncopyable.h"
#include <functional>
#include <string>

using std::string;

class Event_Loop;

class Channel : Noncopyable
{
public:
    typedef std::function<void()> Event_Callback;
    Channel(Event_Loop* loop, int fd);
    ~Channel();

    void handle_event();
    void set_read_callback(const Event_Callback& cb) { read_callback_ = cb; }
    void set_write_callback(const Event_Callback& cb) { write_callback_ = cb; }
    void set_error_callback(const Event_Callback& cb) { error_callback_ = cb; }

    string revents_to_string() const;
    string events_to_string() const;

    int fd() const { return fd_; }
    int events() const { return events_; }
    void set_revents(int revt) { revents_ = revt; }
    bool is_none_event() const { return events_ == k_none_event; }

    void enable_reading()
    {
        events_ |= k_read_event;
        update();
    }

    void enable_writing()
    {
        events_ |= k_write_event;
        update();
    }

    void disable_writing()
    {
        events_ &= !k_write_event;
        update();
    }

    void disable_all()
    {
        events_ = k_none_event;
        update();
    }

    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    Event_Loop* owner_loop() { return loop_; }

private:
    static string events_to_string(int fd, int events);
    void update();

    static const int k_none_event;
    static const int k_read_event;
    static const int k_write_event;

    Event_Loop* loop_;
    const int fd_;
    int events_;    // 关心的IO事件
    int revents_;   // 目前活动的事件
    int index_;

    Event_Callback read_callback_;
    Event_Callback write_callback_;
    Event_Callback error_callback_;
};