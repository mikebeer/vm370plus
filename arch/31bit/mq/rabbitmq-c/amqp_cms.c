/* amqp_cms.c -- what rabbitmq-c needs from cmsrt beyond newlib (9.7) */
#include <time.h>
#include <sys/time.h>
int clock_gettime(clockid_t id, struct timespec *tp)
{
  struct timeval tv;
  (void)id;
  gettimeofday(&tv, 0);
  tp->tv_sec = tv.tv_sec;
  tp->tv_nsec = tv.tv_usec * 1000;
  return 0;
}
