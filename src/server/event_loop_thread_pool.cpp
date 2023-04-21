#include "event_loop_thread_pool.h"
#include "event_loop.h"
#include "event_loop_thread.h"

Event_Loop_Thread_Pool::Event_Loop_Thread_Pool(Event_Loop* base_loop)
    : base_loop_(base_loop),
      started_(false),
      num_threads_(0),
      next_(0)
{
}

Event_Loop_Thread_Pool::~Event_Loop_Thread_Pool()
{
}

void Event_Loop_Thread_Pool::start()
{
    assert(!started_);
    base_loop_->assert_in_loop_thread();
    started_ = true;
    for (int i = 0; i < num_threads_; ++i) {
        Event_Loop_Thread* t = new Event_Loop_Thread();
        threads_.emplace_back(t);
        loops_.emplace_back(t->start_loop());
    }
}

Event_Loop* Event_Loop_Thread_Pool::get_next_loop()
{
    base_loop_->assert_in_loop_thread();
    assert(started_);
    Event_Loop* loop = base_loop_;
    if (!loops_.empty()) {
        // round-robin
        loop = loops_[next_];
        ++next_;
        if (static_cast<size_t>(next_) >= loops_.size())
            next_ = 0;
    }

    return loop;
}