/* CMS green threads (cmsthread.c): replaces newlib's */
#ifndef _SYS__PTHREADTYPES_H_
#define _SYS__PTHREADTYPES_H_
typedef struct cms_thread *pthread_t;
typedef struct { unsigned long stacksize; int detach; void *stackaddr; } pthread_attr_t;
typedef struct { pthread_t owner; int count; int kind; } pthread_mutex_t;
typedef struct { int kind; } pthread_mutexattr_t;
typedef struct { int seq; } pthread_cond_t;
typedef struct { int dummy; } pthread_condattr_t;
typedef unsigned int pthread_key_t;
typedef int pthread_once_t;
#endif
