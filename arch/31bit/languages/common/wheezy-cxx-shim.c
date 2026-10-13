#define _GNU_SOURCE 1
/* wheezy-cxx-shim.c -- glibc functions newer than wheezy's 2.13 that GCC
   13's libstdc++.a calls; linked by wc++ */
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
void *aligned_alloc(size_t al, size_t n)
{ void *p = 0; if (posix_memalign(&p, al < sizeof(void *) ? sizeof(void *) : al, n)) return 0; return p; }
int timespec_get(struct timespec *ts, int base)
{ struct timeval tv; if (base != 1 || gettimeofday(&tv, 0)) return 0;
  ts->tv_sec = tv.tv_sec; ts->tv_nsec = tv.tv_usec * 1000; return base; }
char *secure_getenv(const char *n) { return getenv(n); }
int getentropy(void *buf, size_t len)
{ int fd = open("/dev/urandom", O_RDONLY); ssize_t r; if (fd < 0) return -1;
  r = read(fd, buf, len); close(fd); return r == (ssize_t) len ? 0 : -1; }
char __libc_single_threaded = 1;
/* C23 names that GCC 13's headers (built for glibc 2.38+) bind to */
unsigned long __isoc23_strtoul(const char *s, char **e, int b) { return strtoul(s, e, b); }
long __isoc23_strtol(const char *s, char **e, int b) { return strtol(s, e, b); }
unsigned long long __isoc23_strtoull(const char *s, char **e, int b) { return strtoull(s, e, b); }
long long __isoc23_strtoll(const char *s, char **e, int b) { return strtoll(s, e, b); }
unsigned int arc4random(void)
{ unsigned int r = 0; getentropy(&r, sizeof r); return r; }
/* no _dl_find_object in glibc 2.13, and GCC 13's unwinder has no fallback
   when the symbol exists: find the object and its PT_GNU_EH_FRAME
   (.eh_frame_hdr) with dl_iterate_phdr, as glibc 2.35 does */
#include <link.h>
struct dl_find_object {
  unsigned long long dlfo_flags;
  void *dlfo_map_start, *dlfo_map_end;
  struct link_map *dlfo_link_map;
  void *dlfo_eh_frame;
  unsigned long long reserved[7];
};
struct dlfo_req { unsigned long pc; struct dl_find_object *r; int found; };
static int dlfo_cb(struct dl_phdr_info *info, size_t size, void *data)
{
  struct dlfo_req *q = data;
  unsigned long lo = ~0UL, hi = 0, eh = 0;
  int i, in = 0;
  (void) size;
  for (i = 0; i < info->dlpi_phnum; i++) {
    const ElfW(Phdr) *ph = &info->dlpi_phdr[i];
    unsigned long a = info->dlpi_addr + ph->p_vaddr;
    if (ph->p_type == PT_LOAD) {
      if (a < lo) lo = a;
      if (a + ph->p_memsz > hi) hi = a + ph->p_memsz;
      if (q->pc >= a && q->pc < a + ph->p_memsz) in = 1;
    } else if (ph->p_type == PT_GNU_EH_FRAME)
      eh = a;
  }
  if (!in) return 0;
  q->r->dlfo_flags = 0;
  q->r->dlfo_map_start = (void *) lo;
  q->r->dlfo_map_end = (void *) hi;
  q->r->dlfo_link_map = 0;
  q->r->dlfo_eh_frame = (void *) eh;
  q->found = eh != 0;
  return 1;
}
int _dl_find_object(void *pc, struct dl_find_object *r)
{
  struct dlfo_req q;
  q.pc = (unsigned long) pc & 0x7fffffffUL; q.r = r; q.found = 0;
  dl_iterate_phdr(dlfo_cb, &q);
  return q.found ? 0 : -1;
}
