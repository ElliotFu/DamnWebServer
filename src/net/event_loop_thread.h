#pragma once

#include "base/condition.h"
#include "base/mutex.h"
#include "base/thread.h"
#include "base/noncopyable.h"
#include <functional>
#include <string>

class Event_Loop;

class Event_Loop_Thread : Noncopyable
{
public:
    typedef std::function<void(Event_Loop*)> Thread_Init_Callback;

    Event_Loop_Thread(const Thread_Init_Callback& cb = Thread_Init_Callback(),
                      const std::string& name = std::string());

    ~Event_Loop_Thread();
    Event_Loop* start_loop();

private:
    void thread_func();

    Event_Loop* loop_ GUARDED_BY(mutex_);
    bool existing_;
    Thread thread_;
    Mutex mutex_;
    Condition cond_ GUARDED_BY(mutex_);
    Thread_Init_Callback callback_;
};