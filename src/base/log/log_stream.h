#pragma once

#include "fixed_buffer.h"
#include "base/noncopyable.h"

#include <algorithm>
#include <string>

class General_Template
{
public:
    General_Template()
        : data_(nullptr),
          len_(0)
    {}

    explicit General_Template(const char* data, int len)
        : data_(data),
          len_(len)
    {}

    const char* data_;
    int len_;
};

class Log_Stream : Noncopyable
{
public:
    using Buffer = Fixed_Buffer<SMALL_BUFFER_SIZE>;

    Log_Stream& operator<<(bool v)
    {
        buffer_.append(v ? "1" : "0", 1);
        return *this;
    }

    Log_Stream& operator<<(short v)
    {
        *this << static_cast<int>(v);
        return *this;
    }

    Log_Stream& operator<<(unsigned short v)
    {
        *this << static_cast<unsigned int>(v);
        return *this;
    }

    Log_Stream& operator<<(int v)
    {
        format_integer(v);
        return *this;
    }

    Log_Stream& operator<<(unsigned int v)
    {
        format_integer(v);
        return *this;
    }

    Log_Stream& operator<<(long v)
    {
        format_integer(v);
        return *this;
    }

    Log_Stream& operator<<(unsigned long v)
    {
        format_integer(v);
        return *this;
    }

    Log_Stream& operator<<(long long v)
    {
        format_integer(v);
        return *this;
    }

    Log_Stream& operator<<(unsigned long long v)
    {
        format_integer(v);
        return *this;
    }

    Log_Stream& operator<<(const void* data)
    {
        *this << static_cast<const char*>(data);
        return *this;
    }

    Log_Stream& operator<<(double v)
    {
        if (buffer_.avail() >= MAX_NUMRIC_SIZE) {
            int len = snprintf(buffer_.current(), MAX_NUMRIC_SIZE, "%.12g", v);
            buffer_.add(len);
            return *this;
        }
    }

    Log_Stream& operator<<(float v)
    {
        *this << static_cast<double>(v);
        return *this;
    }

    Log_Stream& operator<<(char v)
    {
        buffer_.append(&v, 1);
        return *this;
    }

    Log_Stream& operator<<(const char* str)
    {
        if (str) {
            buffer_.append(str, strlen(str));
        } else {
            buffer_.append("(null)", 6);
        }
        return *this;
    }

    Log_Stream& operator<<(const unsigned char* str)
    {
        return operator<<(reinterpret_cast<const char*>(str));
    }

    Log_Stream& operator<<(const std::string& v)
    {
        buffer_.append(v.c_str(), v.size());
        return *this;
    }

    Log_Stream& operator<<(const Buffer& v)
    {
        *this << v.to_string();
        return *this;
    }

    // (const char*, int)的重载
    Log_Stream& operator<<(const General_Template& g)
    {
        buffer_.append(g.data_, g.len_);
        return *this;
    }

    void append(const char* data, int len) { buffer_.append(data, len); }
    const Buffer& buffer() const { return buffer_; }
    void resetBuffer() { buffer_.reset(); }

private:
    static constexpr int MAX_NUMRIC_SIZE = 48;

    template <typename T>
    void format_integer(T);

    Buffer buffer_;
};

static const char digits[] = {'9', '8', '7', '6', '5', '4', '3', '2', '1', '0',
                               '1', '2', '3', '4', '5', '6', '7', '8', '9'};

template <typename T>
void Log_Stream::format_integer(T num)
{
    if (buffer_.avail() >= MAX_NUMRIC_SIZE) {
        char* start = buffer_.current();
        char* cur = start;
        const char* zero = digits + 9;
        bool negative = (num < 0); // 是否为负数

        do {
            int remainder = static_cast<int>(num % 10);
            *(cur++) = zero[remainder];
            num = num / 10;
        } while (num != 0);

        if (negative) {
            *(cur++) = '-';
        }
        *cur = '\0';

        std::reverse(start, cur);
        buffer_.add(static_cast<int>(cur - start)); // cur_向后移动
    }
}
