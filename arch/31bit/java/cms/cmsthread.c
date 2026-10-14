/*
 * cmsthread.c -- POSIX threads, signals and a few Linux calls for JamVM on
 * CMS (VM/370plus 9.35).  CMS runs one task, so these are green threads:
 * each has its own stack from the heap, and the running thread gives up the
 * processor only inside these calls -- a mutex or condition wait, sleep,
 * sched_yield, sigsuspend, sigwait, join.  Signals between threads
 * (JamVM suspends threads for GC with pthread_kill) are delivered when the
 * target thread next runs, by calling the handler on its own stack.
 * Contexts are cmsrt's setjmp buffers: R6-R15 (soft float, no FPRs).
 */
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <setjmp.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#define NKEYS 64
#define NSIG_ 33

enum { RUN, READY, WAIT, DEAD };

struct cms_thread {
    jmp_buf ctx;
    int state;
    void *waitobj;              /* mutex, cond, or thread being joined */
    long long deadline;         /* microseconds, 0 = none */
    int timedout;
    void *(*start)(void *);
    void *arg, *retval;
    char *stack;
    size_t stacksize;
    void *keys[NKEYS];
    unsigned int pending, blocked;
    int insigsuspend, sigwaiting, sigwaitmask, gotsig;
    int detached;
    struct cms_thread *next;
};

extern void *cms_main_stack_base, *cms_main_stack_top;

static struct cms_thread mainthr;
static struct cms_thread *cur, *all;
static void (*keydtor[NKEYS])(void *);
static int nkeys;
static void (*handlers[NSIG_])(int);
static unsigned int sigsdefault_ignored;

static void init(void)
{
    if (cur) return;
    mainthr.state = RUN;
    mainthr.stack = cms_main_stack_base;
    mainthr.stacksize = (char *)cms_main_stack_top - (char *)cms_main_stack_base;
    cur = all = &mainthr;
}

static long long now_us(void)
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    return (long long)tv.tv_sec * 1000000 + tv.tv_usec;
}

static void deliver(void)
{
    /* run the handlers of signals that arrived while this thread was off */
    while (cur->pending & ~cur->blocked) {
        int s;
        for (s = 1; s < NSIG_; s++)
            if (cur->pending & ~cur->blocked & (1u << s)) {
                cur->pending &= ~(1u << s);
                if (handlers[s] && handlers[s] != SIG_IGN && handlers[s] != SIG_DFL)
                    handlers[s](s);
            }
    }
}

static void thread_start(void);

/* give the processor to another thread; return when this one runs again */
static void reschedule(void)
{
    struct cms_thread *t, *me = cur;
    for (;;) {
        long long n = 0, soonest = 0;
        t = me->next ? me->next : all;
        for (;;) {
            if (t->state == WAIT && t->deadline) {
                if (!n) n = now_us();
                if (t->deadline <= n) { t->state = READY; t->timedout = 1; }
                else if (!soonest || t->deadline < soonest) soonest = t->deadline;
            }
            if (t->state == READY || (t == me && me->state == RUN)) break;
            t = t->next ? t->next : all;
            if (t == (me->next ? me->next : all)) { t = 0; break; }
        }
        if (t) {
            if (t == me) { me->state = RUN; return; }
            if (setjmp(me->ctx) == 0) {
                cur = t;
                t->state = RUN;
                longjmp(t->ctx, 1);
            }
            /* back in me */
            deliver();
            return;
        }
        if (!soonest) {
            fprintf(stderr, "cmsthread: all threads are waiting -- deadlock\n");
            exit(99);
        }
#ifdef CMS_WATCHDOG
        {   /* test builds: a VM that is only waiting cannot hang the run */
            static long long t0;
            long long n = now_us();
            if (!t0) t0 = n;
            if (n - t0 > CMS_WATCHDOG * 1000000LL) {
                fprintf(stderr, "cmsthread: watchdog: %d s, ending\n", CMS_WATCHDOG);
                exit(98);
            }
        }
#endif
        while (now_us() < soonest) ;        /* nothing to run: wait */
    }
}

static void block(void *obj, long long deadline)
{
    cur->state = WAIT;
    cur->waitobj = obj;
    cur->deadline = deadline;
    cur->timedout = 0;
    reschedule();
    cur->waitobj = 0;
    cur->deadline = 0;
}

static void wake_all(void *obj, int one)
{
    struct cms_thread *t;
    for (t = all; t; t = t->next)
        if (t->state == WAIT && t->waitobj == obj) {
            t->state = READY;
            if (one) return;
        }
}

/* ----------------------------------------------------------- threads */
static void thread_start(void)
{
    void *r;
    deliver();
    r = cur->start(cur->arg);
    pthread_exit(r);
}

int pthread_create(pthread_t *tp, const pthread_attr_t *attr,
                   void *(*fn)(void *), void *arg)
{
    struct cms_thread *t, *l;
    size_t sz = attr && attr->stacksize ? attr->stacksize : 256 * 1024;
    init();
    t = calloc(1, sizeof *t);
    if (!t) return EAGAIN;
    t->stack = malloc(sz);
    if (!t->stack) { free(t); return EAGAIN; }
    t->stacksize = sz;
    t->start = fn;
    t->arg = arg;
    t->detached = attr && attr->detach;
    t->blocked = cur->blocked;
    memset(t->ctx, 0, sizeof t->ctx);
    t->ctx[8] = (int)(long)thread_start;                  /* R14 */
    t->ctx[9] = (int)(long)(t->stack + sz - 96 - 8);       /* R15 */
    *(int *)(t->stack + sz - 96 - 8) = 0;                 /* no back chain */
    t->state = READY;
    for (l = all; l->next; l = l->next) ;
    l->next = t;
    *tp = t;
    return 0;
}

pthread_t pthread_self(void) { init(); return cur; }
int pthread_equal(pthread_t a, pthread_t b) { return a == b; }

void pthread_exit(void *r)
{
    int k;
    for (k = 0; k < nkeys; k++)
        if (cur->keys[k] && keydtor[k]) {
            void *v = cur->keys[k];
            cur->keys[k] = 0;
            keydtor[k](v);
        }
    cur->retval = r;
    cur->state = DEAD;
    wake_all(cur, 0);                       /* joiners */
    if (cur == &mainthr) exit(0);
    reschedule();                            /* never comes back */
    exit(98);
}

int pthread_join(pthread_t t, void **r)
{
    init();
    while (t->state != DEAD) block(t, 0);
    if (r) *r = t->retval;
    return 0;
}

int pthread_detach(pthread_t t) { t->detached = 1; return 0; }

int sched_yield(void)
{
    init();
    cur->state = READY;
    reschedule();
    return 0;
}

int nanosleep(const struct timespec *rq, struct timespec *rem)
{
    long long d;
    init();
    d = now_us() + rq->tv_sec * 1000000LL + rq->tv_nsec / 1000;
    block(&d, d);
    if (rem) rem->tv_sec = rem->tv_nsec = 0;
    return 0;
}

int usleep(useconds_t us)
{
    struct timespec ts = { us / 1000000, (us % 1000000) * 1000 };
    return nanosleep(&ts, 0);
}

unsigned sleep(unsigned s)
{
    struct timespec ts = { s, 0 };
    nanosleep(&ts, 0);
    return 0;
}

/* ----------------------------------------------------------- attributes */
int pthread_attr_init(pthread_attr_t *a) { memset(a, 0, sizeof *a); return 0; }
int pthread_attr_destroy(pthread_attr_t *a) { return 0; }
int pthread_attr_setstacksize(pthread_attr_t *a, size_t s) { a->stacksize = s; return 0; }
int pthread_attr_getstacksize(const pthread_attr_t *a, size_t *s)
{ *s = a->stacksize ? a->stacksize : 256 * 1024; return 0; }
int pthread_attr_setdetachstate(pthread_attr_t *a, int d) { a->detach = d; return 0; }
int pthread_attr_getstack(const pthread_attr_t *a, void **addr, size_t *s)
{ *addr = a->stackaddr; *s = a->stacksize; return 0; }
int pthread_getattr_np(pthread_t t, pthread_attr_t *a)
{
    init();
    a->stackaddr = t->stack;
    a->stacksize = t->stacksize;
    a->detach = t->detached;
    return 0;
}

/* ----------------------------------------------------------- mutexes */
int pthread_mutexattr_init(pthread_mutexattr_t *a) { a->kind = 0; return 0; }
int pthread_mutexattr_settype(pthread_mutexattr_t *a, int k) { a->kind = k; return 0; }
int pthread_mutexattr_destroy(pthread_mutexattr_t *a) { return 0; }

int pthread_mutex_init(pthread_mutex_t *m, const pthread_mutexattr_t *a)
{ m->owner = 0; m->count = 0; m->kind = a ? a->kind : 0; return 0; }
int pthread_mutex_destroy(pthread_mutex_t *m) { return 0; }

int pthread_mutex_lock(pthread_mutex_t *m)
{
    init();
    if (m->owner == cur) { m->count++; return 0; }
    while (m->owner) block(m, 0);
    m->owner = cur;
    m->count = 1;
    return 0;
}

int pthread_mutex_trylock(pthread_mutex_t *m)
{
    init();
    if (m->owner == cur) { m->count++; return 0; }
    if (m->owner) return EBUSY;
    m->owner = cur;
    m->count = 1;
    return 0;
}

int pthread_mutex_unlock(pthread_mutex_t *m)
{
    if (--m->count > 0) return 0;
    m->owner = 0;
    m->count = 0;
    wake_all(m, 1);
    return 0;
}

/* ----------------------------------------------------------- conditions */
int pthread_cond_init(pthread_cond_t *c, const pthread_condattr_t *a) { c->seq = 0; return 0; }
int pthread_cond_destroy(pthread_cond_t *c) { return 0; }

static int condwait(pthread_cond_t *c, pthread_mutex_t *m, long long deadline)
{
    int n = m->count, to;
    m->count = 1;
    pthread_mutex_unlock(m);
    block(c, deadline);
    to = cur->timedout;
    pthread_mutex_lock(m);
    m->count = n;
    return to ? ETIMEDOUT : 0;
}

int pthread_cond_wait(pthread_cond_t *c, pthread_mutex_t *m)
{ init(); return condwait(c, m, 0); }

int pthread_cond_timedwait(pthread_cond_t *c, pthread_mutex_t *m,
                           const struct timespec *ts)
{
    long long d = ts->tv_sec * 1000000LL + ts->tv_nsec / 1000;
    init();
    if (d <= now_us()) return ETIMEDOUT;
    return condwait(c, m, d);
}

int pthread_cond_signal(pthread_cond_t *c) { wake_all(c, 1); return 0; }
int pthread_cond_broadcast(pthread_cond_t *c) { wake_all(c, 0); return 0; }

/* ----------------------------------------------------------- keys */
int pthread_key_create(pthread_key_t *k, void (*d)(void *))
{
    if (nkeys >= NKEYS) return EAGAIN;
    keydtor[nkeys] = d;
    *k = nkeys++;
    return 0;
}
void *pthread_getspecific(pthread_key_t k) { init(); return cur->keys[k]; }
int pthread_setspecific(pthread_key_t k, const void *v)
{ init(); cur->keys[k] = (void *)v; return 0; }

int pthread_once(pthread_once_t *o, void (*f)(void))
{
    if (!*o) { *o = 1; f(); }
    return 0;
}

/* ----------------------------------------------------------- signals */
int sigaction(int s, const struct sigaction *sa, struct sigaction *old)
{
    if (s <= 0 || s >= NSIG_) { errno = EINVAL; return -1; }
    if (old) { memset(old, 0, sizeof *old); old->sa_handler = handlers[s]; }
    if (sa) handlers[s] = sa->sa_handler;
    return 0;
}

_sig_func_ptr signal(int s, _sig_func_ptr h)
{
    _sig_func_ptr o;
    if (s <= 0 || s >= NSIG_) return SIG_ERR;
    o = handlers[s];
    handlers[s] = h;
    return o;
}

int pthread_kill(pthread_t t, int s)
{
    init();
    if (s == 0) return 0;
    t->pending |= 1u << s;
    if (t == cur) { deliver(); return 0; }
    if (t->state == WAIT && (t->insigsuspend ||
        (t->sigwaiting && (t->sigwaitmask & (1u << s)))))
        t->state = READY;
    return 0;
}

int raise(int s) { return pthread_kill(pthread_self(), s); }
int kill(pid_t p, int s) { return raise(s); }

static unsigned int maskof(const sigset_t *set)
{
    return set ? (unsigned int)*set : 0;
}

int pthread_sigmask(int how, const sigset_t *set, sigset_t *old)
{
    init();
    if (old) *old = cur->blocked;
    if (set) {
        if (how == SIG_BLOCK) cur->blocked |= maskof(set);
        else if (how == SIG_UNBLOCK) cur->blocked &= ~maskof(set);
        else cur->blocked = maskof(set);
    }
    deliver();
    return 0;
}

int sigprocmask(int how, const sigset_t *set, sigset_t *old)
{ return pthread_sigmask(how, set, old); }

int sigsuspend(const sigset_t *mask)
{
    unsigned int save;
    init();
    save = cur->blocked;
    cur->blocked = maskof(mask);
    while (!(cur->pending & ~cur->blocked)) {
        cur->insigsuspend = 1;
        block(&cur->insigsuspend, 0);
        cur->insigsuspend = 0;
    }
    deliver();
    cur->blocked = save;
    errno = EINTR;
    return -1;
}

int sigwait(const sigset_t *set, int *sig)
{
    unsigned int m = maskof(set);
    int s;
    init();
    while (!(cur->pending & m)) {
        cur->sigwaiting = 1;
        cur->sigwaitmask = m;
        block(&cur->sigwaiting, 0);
        cur->sigwaiting = 0;
    }
    for (s = 1; s < NSIG_; s++)
        if (cur->pending & m & (1u << s)) {
            cur->pending &= ~(1u << s);
            *sig = s;
            return 0;
        }
    return EINVAL;
}

/* ----------------------------------------------------------- misc */
#include <sys/mman.h>
#include <sys/utsname.h>
#include <fcntl.h>
extern void *cms_fd_take(int, unsigned int *);

/* mmap: anonymous memory, or a private copy of a file (JamVM maps the
   class archives read only) -- both from the heap */
void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off)
{
    char *p;
    /* whole pages, as the kernel gives: JamVM's gcMemRealloc grows a
       mapping in place up to the end of its last page (9.35: without this
       its GC lists overran the next heap block -- freed bytecode) */
    len = (len + 4095) & ~(size_t)4095;
    if (!(flags & MAP_ANONYMOUS) && off == 0) {
        unsigned int have;
        p = cms_fd_take(fd, &have);         /* the file's storage, once */
        if (p && have >= len) return p;
        if (p) {
            char *q = realloc(p, len);
            if (q) { memset(q + have, 0, len - have); return q; }
            free(p);
            errno = ENOMEM; return MAP_FAILED;
        }
    }
    p = malloc(len ? len : 1);
    if (!p) { errno = ENOMEM; return MAP_FAILED; }
    if (flags & MAP_ANONYMOUS) { memset(p, 0, len); return p; }
    if (lseek(fd, off, SEEK_SET) < 0) { free(p); return MAP_FAILED; }
    {
        size_t got = 0; ssize_t r;
        while (got < len && (r = read(fd, p + got, len - got)) > 0) got += r;
        if (got < len) memset(p + got, 0, len - got);
    }
    return p;
}
int munmap(void *a, size_t l) { free(a); return 0; }
int mprotect(void *a, size_t l, int p) { return 0; }

int uname(struct utsname *u)
{
    memset(u, 0, sizeof *u);
    strcpy(u->sysname, "CMS");
    strcpy(u->nodename, "VM370PLUS");
    strcpy(u->release, "VM/370plus");
    strcpy(u->version, "1");
    strcpy(u->machine, "s390");
    return 0;
}

int getpagesize(void) { return 4096; }

/* JamVM scans the extension directories for jars; CMS has none */
struct dirent;
int scandir(const char *d, struct dirent ***list, int (*sel)(const struct dirent *),
            int (*cmp)(const struct dirent **, const struct dirent **))
{ *list = 0; return 0; }
int alphasort(const struct dirent **a, const struct dirent **b) { return 0; }

/* Java needs an absolute user.dir: the accessed disks are "/" ("/fn.ft"
   is a CMS file id, the way cmsrt reads it).  Overrides cmsrt's weak ".". */
char *getcwd(char *buf, size_t n)
{
    if (!buf || n < 2) { errno = ERANGE; return 0; }
    strcpy(buf, "/"); return buf;
}

/* ----------------------------------------------------------- diagnosis */
void cms_diag_dump(void *code, int len)
{
    unsigned char *c = code;
    int i;
    fprintf(stderr, "CMSDIAG code %p len %d thread %p stack %p-%p sp~%p:",
            code, len, (void *)cur, cur ? cur->stack : 0,
            cur ? cur->stack + cur->stacksize : 0, (void *)&i);
    for (i = 0; i < len && i < 48; i++) fprintf(stderr, " %02x", c[i]);
    fprintf(stderr, "\n");
}

void cms_diag_prepare(void *code, int len, void *cd, void *map, const char *name)
{
    char *c = code, *s = cur ? cur->stack : 0, *e = s ? s + cur->stacksize : 0;
    char here;
    if ((s && c + len > s && c < e) || (c + len > (char *)&here - 65536 && c < (char *)&here + 65536))
        fprintf(stderr, "CMSDIAG overlap: %s code %p+%d stack %p-%p cd %p map %p sp %p\n",
                name, code, len, s, e, cd, map, (void *)&here);
}
