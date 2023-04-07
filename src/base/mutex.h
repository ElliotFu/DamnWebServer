// Copied From Muduo
#pragma once

#include "current_thread.h"
#include "noncopyable.h"
#include <cassert>
#include <pthread.h>

// Thread safety annotations {
// https://clang.llvm.org/docs/ThreadSafetyAnalysis.html

// Enable thread safety attributes only with clang.
// The attributes can be safely erased when compiling with other compilers.
#if defined(__clang__) && (!defined(SWIG))
#define THREAD_ANNOTATION_ATTRIBUTE__(x)   __attribute__((x))
#else
#define THREAD_ANNOTATION_ATTRIBUTE__(x)   // no-op
#endif

#define CAPABILITY(x) \
  THREAD_ANNOTATION_ATTRIBUTE__(capability(x))

#define SCOPED_CAPABILITY \
  THREAD_ANNOTATION_ATTRIBUTE__(scoped_lockable)

#define GUARDED_BY(x) \
  THREAD_ANNOTATION_ATTRIBUTE__(guarded_by(x))

#define PT_GUARDED_BY(x) \
  THREAD_ANNOTATION_ATTRIBUTE__(pt_guarded_by(x))

#define ACQUIRED_BEFORE(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(acquired_before(__VA_ARGS__))

#define ACQUIRED_AFTER(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(acquired_after(__VA_ARGS__))

#define REQUIRES(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(requires_capability(__VA_ARGS__))

#define REQUIRES_SHARED(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(requires_shared_capability(__VA_ARGS__))

#define ACQUIRE(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(acquire_capability(__VA_ARGS__))

#define ACQUIRE_SHARED(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(acquire_shared_capability(__VA_ARGS__))

#define RELEASE(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(release_capability(__VA_ARGS__))

#define RELEASE_SHARED(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(release_shared_capability(__VA_ARGS__))

#define TRY_ACQUIRE(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(try_acquire_capability(__VA_ARGS__))

#define TRY_ACQUIRE_SHARED(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(try_acquire_shared_capability(__VA_ARGS__))

#define EXCLUDES(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(locks_excluded(__VA_ARGS__))

#define ASSERT_CAPABILITY(x) \
  THREAD_ANNOTATION_ATTRIBUTE__(assert_capability(x))

#define ASSERT_SHARED_CAPABILITY(x) \
  THREAD_ANNOTATION_ATTRIBUTE__(assert_shared_capability(x))

#define RETURN_CAPABILITY(x) \
  THREAD_ANNOTATION_ATTRIBUTE__(lock_returned(x))

#define NO_THREAD_SAFETY_ANALYSIS \
  THREAD_ANNOTATION_ATTRIBUTE__(no_thread_safety_analysis)

// End of thread safety annotations

#ifdef CHECK_PTHREAD_RETURN_VALUE

#ifdef NDEBUG
__BEGIN_DECLS
extern void __assert_perror_fail (int errnum,
                                  const char *file,
                                  unsigned int line,
                                  const char *function)
    noexcept __attribute__ ((__noreturn__));
__END_DECLS
#endif

#define MCHECK(ret) ({ __typeof__ (ret) errnum = (ret);         \
                       if (__builtin_expect(errnum != 0, 0))    \
                         __assert_perror_fail (errnum, __FILE__, __LINE__, __func__);})

#else  // CHECK_PTHREAD_RETURN_VALUE

#define MCHECK(ret) ({ __typeof__ (ret) errnum = (ret);         \
                       assert(errnum == 0); (void) errnum;})

#endif // CHECK_PTHREAD_RETURN_VALUE

class CAPABILITY("mutex") Mutex : Noncopyable
{
public:
    Mutex() : holder_(0) { MCHECK(pthread_mutex_init(&mutex_, NULL)); }

    ~Mutex()
    {
        assert(holder_ == 0);
        MCHECK(pthread_mutex_destroy(&mutex_));
    }

    // must be called when locked, i.e. for assertion
    bool is_locked_by_this_thread() const
    {
        return holder_ == Current_Thread::tid();
    }

    void assert_locked() const ASSERT_CAPABILITY(this)
    {
        assert(is_locked_by_this_thread());
    }

    // internal usage

    void lock() ACQUIRE()
    {
        MCHECK(pthread_mutex_lock(&mutex_));
        assign_holder();
    }

    void unlock() RELEASE()
    {
        unassign_holder();
        MCHECK(pthread_mutex_unlock(&mutex_));
    }

    pthread_mutex_t* get_pthread_mutex() /* non-const */
    {
        return &mutex_;
    }

private:
    friend class Condition;
    pthread_mutex_t mutex_;
    pid_t holder_;

    class Unassign_Guard : Noncopyable
    {
    public:
        explicit Unassign_Guard(Mutex& owner) : owner_(owner)
        {
            owner_.unassign_holder();
        }

        ~Unassign_Guard()
        {
            owner_.assign_holder();
        }

    private:
        Mutex& owner_;
    };

    void unassign_holder() { holder_ = 0; }
    void assign_holder() { holder_ = Current_Thread::tid(); }
};

class SCOPED_CAPABILITY Mutex_Lock_Guard : Noncopyable
{
public:
    explicit Mutex_Lock_Guard(Mutex& mutex) ACQUIRE(mutex) : mutex_(mutex)
    {
        mutex_.lock();
    }

    ~Mutex_Lock_Guard() RELEASE()
    {
        mutex_.unlock();
    }

private:
    Mutex& mutex_;
};