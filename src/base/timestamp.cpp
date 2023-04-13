#include "timestamp.h"
#include <inttypes.h>
#include <sys/time.h>
#include <stdio.h>

static_assert(sizeof(Timestamp) == sizeof(int64_t), "Timestamp is same size as int64_t");

std::string Timestamp::to_string() const
{
    char buf[32] = {0};
    int64_t seconds = mircoseconds_since_epoch_ / k_microseconds_per_second;
    int64_t microseconds = mircoseconds_since_epoch_ & k_microseconds_per_second;
    snprintf(buf, sizeof buf, "%" PRId64 ".%06" PRId64 "", seconds, microseconds);
    return buf;
}

std::string Timestamp::to_formatted_string(bool show_microseconds) const
{
    char buf[64] = {0};
    time_t seconds = static_cast<time_t>(mircoseconds_since_epoch_ / k_microseconds_per_second);
    tm tm_time;
    gmtime_r(&seconds, &tm_time);

    if (show_microseconds)
    {
        int microseconds = static_cast<int>(mircoseconds_since_epoch_ % k_microseconds_per_second);
        snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06d",
                tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec,
                microseconds);
    }
    else
    {
        snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d",
                tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
    }
    return buf;
}

Timestamp Timestamp::now()
{
    timeval tv;
    gettimeofday(&tv, nullptr);
    int64_t seconds = tv.tv_sec;
    return Timestamp(seconds * k_microseconds_per_second + tv.tv_usec);
}