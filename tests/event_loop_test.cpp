#include "server/event_loop.h"
#include "server/channel.h"
#include <string.h>
#include <sys/timerfd.h>
#include <unistd.h>

Event_Loop* g_loop;

void timeout()
{
    printf("Timeout!\n");
    g_loop->quit();
}

int main()
{
    Event_Loop loop;
    g_loop = &loop;
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    Channel channel(&loop, timerfd);
    channel.set_read_callback(timeout);
    channel.enable_reading();

    itimerspec howlong;
    bzero(&howlong, sizeof howlong);
    howlong.it_value.tv_sec = 5;
    ::timerfd_settime(timerfd, 0, &howlong, nullptr);
    loop.loop();
    ::close(timerfd);
    return 0;
}