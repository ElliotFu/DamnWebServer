#pragma once

#include "fixed_buffer.h"
#include "base/thread.h"

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class Async_Logging
{
public:
    Async_Logging(const std::string& basename,
                  off_t roll_size,
                  int flush_interval = 3);

    ~Async_Logging()
    {
        if (running_)
            stop();
    }

    void append(const char* logling, int len);

    void start()
    {
        running_ = true;
        thread_.start();
    }

    void stop()
    {
        running_ = false;
        cond_.notify_one();
        thread_.join();
    }

private:
    using Buffer = Fixed_Buffer<LARGE_BUFFER_SIZE>;
    using Buffer_Vector = std::vector<std::unique_ptr<Buffer>>;
    using Buffer_Ptr = std::unique_ptr<Buffer>; // ??

    void thread_func();
    const int flush_interval_;
    std::atomic<bool> running_;
    const std::string basename_;
    const off_t roll_size_;
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;

    Buffer_Ptr current_buffer_;
    Buffer_Ptr next_buffer_;
    Buffer_Vector buffers_;
};