/* pthread.h -- green threads for JamVM on CMS (VM/370plus 9.35, cmsthread.c).
   One CMS task; threads switch only inside these calls (and sched_yield,
   nanosleep, sigsuspend, sigwait). */
#ifndef CMS_PTHREAD_H
#define CMS_PTHREAD_H
#include <sys/types.h>
#include <time.h>
#include <sched.h>
#include <sys/_pthreadtypes.h>
#define PTHREAD_MUTEX_INITIALIZER {0, 0, 0}
#define PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP {0, 0, 1}
#define PTHREAD_COND_INITIALIZER {0}
#define PTHREAD_ONCE_INIT 0
#define PTHREAD_CREATE_JOINABLE 0
#define PTHREAD_CREATE_DETACHED 1
#define PTHREAD_MUTEX_NORMAL 0
#define PTHREAD_MUTEX_RECURSIVE 1
#define PTHREAD_MUTEX_ERRORCHECK 2
#define PTHREAD_STACK_MIN 16384
int pthread_create(pthread_t *, const pthread_attr_t *, void *(*)(void *), void *);
pthread_t pthread_self(void);
int pthread_equal(pthread_t, pthread_t);
int pthread_join(pthread_t, void **);
int pthread_detach(pthread_t);
void pthread_exit(void *) __attribute__((noreturn));
int pthread_kill(pthread_t, int);
int pthread_attr_init(pthread_attr_t *);
int pthread_attr_destroy(pthread_attr_t *);
int pthread_attr_setstacksize(pthread_attr_t *, size_t);
int pthread_attr_getstacksize(const pthread_attr_t *, size_t *);
int pthread_attr_setdetachstate(pthread_attr_t *, int);
int pthread_attr_getstack(const pthread_attr_t *, void **, size_t *);
int pthread_getattr_np(pthread_t, pthread_attr_t *);
int pthread_mutex_init(pthread_mutex_t *, const pthread_mutexattr_t *);
int pthread_mutex_destroy(pthread_mutex_t *);
int pthread_mutex_lock(pthread_mutex_t *);
int pthread_mutex_trylock(pthread_mutex_t *);
int pthread_mutex_unlock(pthread_mutex_t *);
int pthread_mutexattr_init(pthread_mutexattr_t *);
int pthread_mutexattr_settype(pthread_mutexattr_t *, int);
int pthread_mutexattr_destroy(pthread_mutexattr_t *);
int pthread_cond_init(pthread_cond_t *, const pthread_condattr_t *);
int pthread_cond_destroy(pthread_cond_t *);
int pthread_cond_wait(pthread_cond_t *, pthread_mutex_t *);
int pthread_cond_timedwait(pthread_cond_t *, pthread_mutex_t *, const struct timespec *);
int pthread_cond_signal(pthread_cond_t *);
int pthread_cond_broadcast(pthread_cond_t *);
int pthread_key_create(pthread_key_t *, void (*)(void *));
void *pthread_getspecific(pthread_key_t);
int pthread_setspecific(pthread_key_t, const void *);
int pthread_once(pthread_once_t *, void (*)(void));
#endif
