#pragma once

#include "fixed_buffer.h"
#include "base/noncopyable.h"

#include <string>

class Log_Stream : Noncopyable
{
public:
    using Buffer = Fixed_Buffer<SMALL_BUFFER_SIZE>;

    Log_Stream& operator<<(bool v)
    {
        buffer_.append(v ? "1" : "0", 1);
        return *this;
    }

    Log_Stream& operator<<(short);
    Log_Stream& operator<<(unsigned short);
    Log_Stream& operator<<(int);
    Log_Stream& operator<<(unsigned int);
    Log_Stream& operator<<(long);
    Log_Stream& operator<<(unsigned long);
    Log_Stream& operator<<(long long);
    Log_Stream& operator<<(unsigned long long);

    Log_Stream& operator<<(const void*);

    Log_Stream& operator<<(double);
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

    void append(const char* data, int len) { buffer_.append(data, len); }
    const Buffer& buffer() const { return buffer_; }
    void resetBuffer() { buffer_.reset(); }

private:
    static constexpr int MAX_NUMRIC_SIZE = 48;

    template <typename T>
    void format_integer(T);

    Buffer buffer_;
};