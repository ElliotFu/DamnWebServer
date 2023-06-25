#include "base/log/async_logging.h"
#include "base/log/logger.h"
#include "base/timestamp.h"

#include <stdio.h>
#include <sys/resource.h>
#include <unistd.h>

off_t k_roll_size = 500*1000*1000;

Async_Logging* g_async_log = NULL;

void async_output(const char* msg, int len)
{
    g_async_log->append(msg, len);
}

void bench(bool long_log)
{
    Logger::set_output(async_output);
    int cnt = 0;
    const int k_batch = 1000;
    std::string empty = " ";
    std::string long_str(3000, 'X');
    long_str += " ";

    for (int t = 0; t < 30; ++t) {
        Timestamp start = Timestamp::now();
        for (int i = 0; i < k_batch; ++i) {
            LOG_INFO << "Hello 0123456789" << " abcdefghijklmnopqrstuvwxyz "
                    << (long_log ? long_str : empty)
                    << cnt;
            ++cnt;
        }
        Timestamp end = Timestamp::now();
        printf("%f\n", time_difference(end, start)*1000000/k_batch);
        struct timespec ts = { 0, 500*1000*1000 };
        nanosleep(&ts, NULL);
    }
}

int main(int argc, char* argv[])
{
    {
        // set max virtual memory to 2GB.
        size_t k_one_GB = 1000*1024*1024;
        rlimit rl = { 2*k_one_GB, 2*k_one_GB };
        setrlimit(RLIMIT_AS, &rl);
    }

  printf("pid = %d\n", getpid());

  char name[256] = { '\0' };
  strncpy(name, argv[0], sizeof name - 1);
  Async_Logging log(::basename(name), k_roll_size);
  log.start();
  g_async_log = &log;

  bool long_log = argc > 1;
  bench(long_log);
}
