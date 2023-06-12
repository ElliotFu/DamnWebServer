#pragma once

#include <string.h>

constexpr int SMALL_BUFFER_SIZE = 4000;
constexpr int LARGE_BUFFER_SIZE = 4000 * 1000;

template<int SIZE>
class Fixed_Buffer
{
public:
    Fixed_Buffer() : cur_(data_) {}
    ~Fixed_Buffer() = default

    void append(const char* buf, size_t len)
    {
        if (static_cast<size_t>(avail()) > len) {
            memcpy(cur_, buf, len);
            cur_ += len;
        }
    }

    const char* data() const { return data_; }
    int length() const { return static_cast<int>(end() - data_); }
    int avail() const { return static_cast<int>(end() - cur_); }
    char* current() { return cur_; }
    void add(size_t len) { cur_ += len; }
    void reset() { cur_ = data_; }
    void bzero() { ::bzero(data_, sizeof(data_)); }

    std::string to_string() const { return std::string(data_, length()); }

private:
    const char* end() const { return data_ + sizeof(data_); }

    char data_[SIZE];
    char* cur_;
};