#include "event_loop_thread.h"
#include "event_loop.h"
Event_Loop_Thread::Event_Loop_Thread(const Thread_Init_Callback& cb, const std::string& name)
    : loop_(nullptr),
      existing_(false),
      thread_(std::bind(&Event_Loop_Thread::thread_func, this), name),
      mutex_(),
      cond_(mutex_),
      callback_(cb)
{
}

Event_Loop_Thread::~Event_Loop_Thread()
{
    existing_ = true;
    if (loop_ != nullptr) {
        loop_->quit();
        thread_.join();
    }
}

Event_Loop* Event_Loop_Thread::start_loop()
{
    assert(!thread_.started());
    thread_.start();
    Event_Loop* loop = nullptr;
    {
        Mutex_Lock_Guard lock(mutex_);
        while (loop_ == nullptr) {
            cond_.wait();
        }
        loop = loop_;
    }

    return loop;
}

void Event_Loop_Thread::thread_func()
{
    Event_Loop loop;
    if (callback_)
        callback_(&loop);

    {
        Mutex_Lock_Guard lock(mutex_);
        loop_ = &loop;
        cond_.notify();
    }

    loop.loop();
    Mutex_Lock_Guard lock(mutex_);
    loop_ = nullptr;
}