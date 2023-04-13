#pragma once

#include <boost/operators.hpp>

class Timestamp : public boost::equality_comparable<Timestamp>,
                  public boost::less_than_comparable<Timestamp>
{
public:
    Timestamp() : mircoseconds_since_epoch_(0) {}
    explicit Timestamp(int64_t arg) : mircoseconds_since_epoch_(arg) {}

    void swap(Timestamp& that) { std::swap(mircoseconds_since_epoch_, that.mircoseconds_since_epoch_); }
    std::string to_string() const;
    std::string to_formatted_string(bool show_micro_seconds = true) const;
    bool valid() const { return mircoseconds_since_epoch_ > 0; }
    int64_t microseconds_since_epoch() const { return mircoseconds_since_epoch_; }
    time_t seconds_since_epoch() const
    {
        return static_cast<time_t>(mircoseconds_since_epoch_ / k_microseconds_per_second);
    }

    static Timestamp now();
    static Timestamp invalid() { return Timestamp(); }
    static Timestamp from_unix_time(time_t t) { return from_unix_time(t, 0); }
    static Timestamp from_unix_time(time_t t, int microseconds)
    {
        return Timestamp(static_cast<int64_t>(t) * k_microseconds_per_second + microseconds);
    }
    static constexpr int k_microseconds_per_second = 1000 * 1000;

private:
    int64_t mircoseconds_since_epoch_;
};

inline bool operator<(const Timestamp& lhs, const Timestamp& rhs)
{
    return lhs.microseconds_since_epoch() < rhs.microseconds_since_epoch();
}

inline bool operator==(const Timestamp& lhs, const Timestamp& rhs)
{
    return lhs.microseconds_since_epoch() == rhs.microseconds_since_epoch();
}

inline double time_difference(Timestamp high, Timestamp low)
{
    int64_t diff = high.microseconds_since_epoch() - low.microseconds_since_epoch();
    return static_cast<double>(diff) / Timestamp::k_microseconds_per_second;
}

inline Timestamp add_time(Timestamp timestamp, double seconds)
{
    int64_t delta = static_cast<int64_t>(seconds * Timestamp::k_microseconds_per_second);
    return Timestamp(timestamp.microseconds_since_epoch() + delta);
}