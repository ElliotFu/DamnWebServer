// Copied from muduo

#pragma once

#include "mutex.h"
#include <pthread.h>

class Condition : Noncopyable
{
public:
    explicit Condition(Mutex& mutex) : mutex_(mutex)
    {
        MCHECK(pthread_cond_init(&pcond_, NULL));
    }

    ~Condition()
    {
        MCHECK(pthread_cond_destroy(&pcond_));
    }

    void wait()
    {
        Mutex::Unassign_Guard ug(mutex_);
        MCHECK(pthread_cond_wait(&pcond_, mutex_.get_pthread_mutex()));
    }

    // returns true if time out, false otherwise.
    bool wait_for_seconds(double seconds);

    void notify()
    {
        MCHECK(pthread_cond_signal(&pcond_));
    }

    void notify_all()
    {
        MCHECK(pthread_cond_broadcast(&pcond_));
    }

private:
    Mutex& mutex_;
    pthread_cond_t pcond_;
};