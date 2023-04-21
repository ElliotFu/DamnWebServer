#pragma once

class Timer;

class Timer_ID
{
public:
    Timer_ID() : timer_(nullptr), sequence_(0) {}
    Timer_ID(Timer* timer, signed long seq) : timer_(timer), sequence_(seq) {}

    friend class Timer_Queue;

private:
    Timer* timer_;
    signed long sequence_;
};