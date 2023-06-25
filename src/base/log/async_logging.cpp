#include "async_logging.h"
#include "log_file.h"

Async_Logging::Async_Logging(const std::string& basename,
                             off_t roll_size,
                             int flush_interval)
    : flush_interval_(flush_interval),
      running_(false),
      basename_(basename),
      roll_size_(roll_size),
      thread_(std::bind(&Async_Logging::thread_func, this), "Logging"),
      mutex_(),
      cond_(),
      current_buffer_(new Buffer),
      next_buffer_(new Buffer),
      buffers_()
{
    current_buffer_->bzero();
    next_buffer_->bzero();
    buffers_.reserve(16);
}

void Async_Logging::append(const char* logline, int len)
{
    std::lock_guard<std::mutex> lock(mutex_);
    // 缓冲区剩余空间足够则直接写入
    if (current_buffer_->avail() > len) {
        current_buffer_->append(logline, len);
    } else {
        // 当前缓冲区空间不够，将新信息写入备用缓冲区
        buffers_.emplace_back(std::move(current_buffer_));
        if (next_buffer_) {
            current_buffer_ = std::move(next_buffer_);
        } else {
            // 备用缓冲区也不够时，重新分配缓冲区，这种情况很少见
            current_buffer_.reset(new Buffer);
        }
        current_buffer_->append(logline, len);
        // 唤醒写入磁盘的后端线程
        cond_.notify_one();
    }
}

void Async_Logging::thread_func()
{
    Log_File output(basename_, roll_size_, false);
    Buffer_Ptr buffer1(new Buffer);
    Buffer_Ptr buffer2(new Buffer);
    buffer1->bzero();
    buffer2->bzero();
    // 缓冲区数组置为16个，用于和前端缓冲区数组进行交换
    Buffer_Vector buffers_to_write;
    buffers_to_write.reserve(16);
    while (running_)
    {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (buffers_.empty()) {
                // 等待三秒也会接触阻塞
                cond_.wait_for(lock, std::chrono::seconds(3));
            }

            // 此时正使用得buffer也放入buffer数组中（没写完也放进去，避免等待太久才刷新一次）
            buffers_.emplace_back(std::move(current_buffer_));
            // 归还正使用缓冲区
            current_buffer_ = std::move(buffer1);
            // 后端缓冲区和前端缓冲区交换
            buffers_to_write.swap(buffers_);
            if (!next_buffer_) {
                next_buffer_ = std::move(buffer2);
            }
        }

        // 遍历所有 buffer，将其写入文件
        for (const auto& buffer : buffers_to_write) {
            output.append(buffer->data(), buffer->length());
        }

        // 只保留两个缓冲区
        if (buffers_to_write.size() > 2) {
            buffers_to_write.resize(2);
        }

        // 归还newBuffer1缓冲区
        if (!buffer1) {
            buffer1 = std::move(buffers_to_write.back());
            buffers_to_write.pop_back();
            buffer1->reset();
        }

        // 归还newBuffer2缓冲区
        if (!buffer2) {
            buffer2 = std::move(buffers_to_write.back());
            buffers_to_write.pop_back();
            buffer2->reset();
        }

        buffers_to_write.clear(); // 清空后端缓冲区队列
        output.flush(); //清空文件缓冲区
    }
    output.flush();
}
