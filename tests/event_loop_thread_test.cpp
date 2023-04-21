#include "net/event_loop.h"
#include "net/event_loop_thread.h"
#include <stdio.h>

void run_in_thread()
{
    printf("run_in_thread(): pid = %d, tid = %d\n", getpid(), Current_Thread::tid());
}

int main()
{
    printf("main(): pid = %d, tid = %d\n", getpid(), Current_Thread::tid());
    Event_Loop_Thread loop_thread;
    Event_Loop* loop = loop_thread.start_loop();
    loop->run_in_loop(run_in_thread);
    sleep(1);
    loop->run_after(2, run_in_thread);
    sleep(3);
    loop->quit();

    printf("exit main().\n");
}