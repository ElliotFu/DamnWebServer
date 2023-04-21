#pragma once

#include "base/noncopyable.h"
#include <arpa/inet.h>
#include <vector>
#include <memory>

class Event_Loop;
class Event_Loop_Thread;

class Event_Loop_Thread_Pool : Noncopyable
{
public:
    Event_Loop_Thread_Pool(Event_Loop* base_loop);
    ~Event_Loop_Thread_Pool();
    void set_thread_num(int num_threads) { num_threads_ = num_threads; }
    void start();
    Event_Loop* get_next_loop();

private:
    Event_Loop* base_loop_;
    bool started_;
    int num_threads_;
    int next_;
    std::vector<std::unique_ptr<Event_Loop_Thread>> threads_;
    std::vector<Event_Loop*> loops_;
};