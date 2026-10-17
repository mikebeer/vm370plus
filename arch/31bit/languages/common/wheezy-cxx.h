/* wheezy-cxx.h -- what GCC 13's libstdc++ headers expect from a newer
   glibc than wheezy's 2.13 (C11/C++11 additions); wc++ includes it first */
#ifndef WHEEZY_CXX_H
#define WHEEZY_CXX_H
/* glibc 2.13's math.h declares int isinf(double) and isnan(double) */
#define _GLIBCXX_HAVE_OBSOLETE_ISINF 1
#define _GLIBCXX_HAVE_OBSOLETE_ISNAN 1
#include <stddef.h>
#include <time.h>
#ifdef __cplusplus
extern "C" {
#endif
void *aligned_alloc(size_t, size_t);
#ifndef TIME_UTC
#define TIME_UTC 1
#endif
int timespec_get(struct timespec *, int);
/* glibc 2.30 clock-selecting waits; GCC 13's <mutex>, <condition_variable>
   and <shared_mutex> call them (shim: converted to CLOCK_REALTIME waits) */
#include <pthread.h>
int pthread_cond_clockwait(pthread_cond_t *, pthread_mutex_t *, clockid_t,
                           const struct timespec *);
int pthread_mutex_clocklock(pthread_mutex_t *, clockid_t, const struct timespec *);
int pthread_rwlock_clockrdlock(pthread_rwlock_t *, clockid_t, const struct timespec *);
int pthread_rwlock_clockwrlock(pthread_rwlock_t *, clockid_t, const struct timespec *);
#ifdef __cplusplus
}
#endif
#endif
