#pragma once

#include "count_down_latch.h"
#include <atomic>
#include <functional>
#include <memory>
#include <pthread.h>
#include <string>

using std::string;

class Thread : Noncopyable
{
public:
    typedef std::function<void ()> Thread_Func;

    explicit Thread(Thread_Func, const string& name = string());
    ~Thread();

    void start();
    int join();
    bool started() const { return started_; }
    pid_t tid() const { return tid_; }
    const string& name() const { return name_; }

private:
    void set_default_name();

    bool started_;
    bool joined_;
    pthread_t pthread_id_;
    pid_t tid_;
    Thread_Func func_;
    string name_;
    Count_Down_Latch latch_;

    static std::atomic_int32_t num_created_;
};