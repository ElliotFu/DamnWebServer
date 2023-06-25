#include "logger.h"
#include "base/current_thread.h"

namespace Thread_Info
{
    thread_local char t_errno_buf[512];
    thread_local char t_time[64];
    thread_local time_t t_last_second;
};

const char* get_errno_msg(int saved_errno)
{
    return strerror_r(saved_errno,
                      Thread_Info::t_errno_buf,
                      sizeof(Thread_Info::t_errno_buf));
}

const char* get_level_name[Logger::Log_Level::LEVEL_COUNT]
{
    "TRACE ",
    "DEBUG ",
    "INFO ",
    "WARN ",
    "ERROR ",
    "FATAL ",
};

Logger::Log_Level init_log_level()
{
    return Logger::INFO;
}

Logger::Log_Level g_log_level = init_log_level();

static void default_output(const char* data, int len)
{
    ::fwrite(data, len, sizeof(char), stdout);
}

static void default_flush()
{
    ::fflush(stdout);
}

Logger::Output_Func g_output = default_output;
Logger::Flush_Func g_flush = default_flush;

Logger::Impl::Impl(Log_Level level, int saved_errno, const char* file, int line)
    : time_(Timestamp::now()),
      stream_(),
      level_(level),
      line_(line),
      basename_(file)
{
    format_time();
    stream_ << General_Template(get_level_name[level], 6);
    if (saved_errno != 0) {
        stream_ << get_errno_msg(saved_errno) << " (errno = " << saved_errno << ") ";
    }
}

void Logger::Impl::format_time()
{
    Timestamp now = Timestamp::now();
    time_t seconds = static_cast<time_t>(now.microseconds_since_epoch() / Timestamp::k_microseconds_per_second);
    int microseconds = static_cast<int>(now.microseconds_since_epoch() % Timestamp::k_microseconds_per_second);

    struct tm *tm_time = localtime(&seconds);
    // 写入此线程存储的时间buf中
    snprintf(Thread_Info::t_time, sizeof(Thread_Info::t_time), "%4d/%02d/%02d %02d:%02d:%02d",
        tm_time->tm_year + 1900,
        tm_time->tm_mon + 1,
        tm_time->tm_mday,
        tm_time->tm_hour,
        tm_time->tm_min,
        tm_time->tm_sec);
    // 更新最后一次时间调用
    Thread_Info::t_last_second = seconds;

    char buf[32] = {0};
    snprintf(buf, sizeof(buf), "%06d ", microseconds);
    stream_ << General_Template(Thread_Info::t_time, 17) << General_Template(buf, 7);
}

void Logger::Impl::finish()
{
    stream_ << " - " << General_Template(basename_.data_, basename_.size_)
            << ':' << line_ << '\n';
}

// level默认为INFO等级
Logger::Logger(const char* file, int line)
    : impl_(INFO, 0, file, line)
{
}

Logger::Logger(const char* file, int line, Logger::Log_Level level)
    : impl_(level, 0, file, line)
{
}

// 可以打印调用函数
Logger::Logger(const char* file, int line, Logger::Log_Level level, const char* func)
  : impl_(level, 0, file, line)
{
    impl_.stream_ << func << ' ';
}


Logger::~Logger()
{
    impl_.finish();
    // 获取buffer(stream_.buffer_)
    const Log_Stream::Buffer& buf(stream().buffer());
    // 输出(默认向终端输出)
    g_output(buf.data(), buf.length());
    // FATAL情况终止程序
    if (impl_.level_ == FATAL)
    {
        g_flush();
        abort();
    }
}

void Logger::set_log_level(Logger::Log_Level level)
{
    g_log_level = level;
}

void Logger::set_output(Output_Func out)
{
    g_output = out;
}

void Logger::set_flush(Flush_Func flush)
{
    g_flush = flush;
}
