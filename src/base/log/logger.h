#pragma once

#include "log_stream.h"
#include "base/timestamp.h"

#include <functional>
#include <string.h>

class Source_File
{
public:
    explicit Source_File(const char* filename) : data_(filename)
    {
        /**
         * 找出data中出现/最后一次的位置，从而获取具体的文件名
         * 2022/10/26/test.log
         */
        const char* slash = strrchr(filename, '/');
        if (slash) {
            data_ = slash + 1;
        }
        size_ = static_cast<int>(strlen(data_));
    }

    const char* data_;
    int size_;
};

class Logger
{
public:
    using Output_Func = std::function<void(const char* msg, int len)>;
    using Flush_Func = std::function<void()>;
    enum Log_Level
    {
        TRACE,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL,
        LEVEL_COUNT,
    };

    Logger(const char* file, int line);
    Logger(const char* file, int line, Log_Level level);
    Logger(const char* file, int line, Log_Level level, const char* func);
    ~Logger();

    Log_Stream& stream() { return impl_.stream_; }

    static void set_log_level(Log_Level level);
    static void set_output(Output_Func);
    static void set_flush(Flush_Func);

private:
    class Impl
    {
    public:
        using Log_Level = Logger::Log_Level;
        Impl(Log_Level level, int saved_errno, const char* file, int line);
        void format_time();
        void finish();

        Timestamp time_;
        Log_Stream stream_;
        Log_Level level_;
        int line_;
        Source_File basename_;
    };

    // Logger's member variable
    Impl impl_;
};

extern Logger::Log_Level g_log_level;
inline Logger::Log_Level log_level() {
    return g_log_level;
}

// 获取errno信息
const char* get_errno_msg(int saved_errno);

// 当日志等级小于对应等级才会输出
#define LOG_DEBUG if (log_level() <= Logger::DEBUG) \
  Logger(__FILE__, __LINE__, Logger::DEBUG, __func__).stream()
#define LOG_INFO if (log_level() <= Logger::INFO) \
  Logger(__FILE__, __LINE__).stream()
#define LOG_WARN Logger(__FILE__, __LINE__, Logger::WARN).stream()
#define LOG_ERROR Logger(__FILE__, __LINE__, Logger::ERROR).stream()
#define LOG_FATAL Logger(__FILE__, __LINE__, Logger::FATAL).stream()