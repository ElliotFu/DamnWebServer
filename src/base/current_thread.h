// copied from muduo
#pragma once

namespace Current_Thread
{
// internal
extern __thread int t_cached_tid;
extern __thread char t_tid_string[32];
extern __thread int t_tid_string_length;
extern __thread const char* t_thread_name;
void cache_tid();
inline int tid() {
    if (__builtin_expect(t_cached_tid == 0, 0)) {
        cache_tid();
    }
    return t_cached_tid;
}

inline const char* tid_string()  // for logging
{
    return t_tid_string;
}

inline int tid_string_length()  // for logging
{
    return t_tid_string_length;
}

inline const char* name() { return t_thread_name; }
}
