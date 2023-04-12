#include "thread.h"
#include "current_thread.h"
#include <iostream>
#include <sys/prctl.h>
#include <sys/unistd.h>
#include <sys/syscall.h>

namespace Current_Thread {
__thread int t_cached_tid = 0;
__thread char t_tid_string[32];
__thread int t_tid_string_length = 6;
__thread const char* t_thread_name = "default";
}

pid_t gettid() { return static_cast<pid_t>( syscall(SYS_gettid)); }

struct Thread_Data {
    typedef Thread::Thread_Func Thread_Func;
    Thread_Func func_;
    string name_;
    pid_t* tid_;
    Count_Down_Latch* latch_;

    Thread_Data(Thread_Func func, const string& name, pid_t* tid, Count_Down_Latch* latch)
        : func_(std::move(func)), name_(name), tid_(tid), latch_(latch)
    {}

    void run_in_thread()
    {
        *tid_ = Current_Thread::tid();
        tid_ = nullptr;
        latch_->count_down();
        latch_ = nullptr;

        Current_Thread::t_thread_name = name_.empty() ? "thread" : name_.c_str();
        ::prctl(PR_SET_NAME, Current_Thread::t_thread_name);
        func_();
        Current_Thread::t_thread_name = "finished";
    }
};

void* start_thread(void* obj)
{
    Thread_Data* data = static_cast<Thread_Data*>(obj);
    data->run_in_thread();
    delete data;
    return nullptr;
}

void Current_Thread::cache_tid()
{
    if (t_cached_tid == 0) {
        t_cached_tid = gettid();
        t_tid_string_length = snprintf(t_tid_string, sizeof t_tid_string, "%5d ", t_cached_tid);
    }
}

std::atomic_int32_t Thread::num_created_(0);

Thread::Thread(Thread_Func func, const string& n)
    : started_(false), joined_(false), pthread_id_(0),
      tid_(0), func_(std::move(func)), name_(n), latch_(1)
{
    set_default_name();
}

Thread::~Thread()
{
    if (started_ && !joined_)
        pthread_detach(pthread_id_);
}

void Thread::set_default_name()
{
    int num = num_created_.fetch_add(1, std::memory_order_relaxed);
    if (name_.empty()) {
        char buf[32];
        snprintf(buf, sizeof buf, "Thread%d", num);
        name_ = buf;
    }
}

void Thread::start()
{
    assert(!started_);
    started_ = true;
    Thread_Data* data = new Thread_Data(func_, name_, &tid_, &latch_);
    if (pthread_create(&pthread_id_, nullptr, &start_thread, data)) {
        started_ = false;
        delete data;
    } else {
        latch_.wait();
        assert(tid_ > 0);
    }
}

int Thread::join() {
    assert(started_);
    assert(!joined_);
    joined_ = true;
    return pthread_join(pthread_id_, nullptr);
}