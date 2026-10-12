/* cxxglibc.c -- what the 32-bit s390 libstdc++ (built for glibc) asks of
 * its C library, on newlib + cmsrt (VM/370plus CMS, 9.21).
 *
 * Locales: only "C".  libstdc++'s GNU locale model reads the ctype tables
 * straight out of glibc's struct __locale_struct, so __newlocale("C")
 * returns one laid out as glibc's, with glibc's ctype tables (ASCII:
 * M5f programs are ASCII inside).  The *_l functions ignore the locale.
 * Threads: none (pthread_* are single-thread stubs).
 * Exceptions: none -- a throw ends the program (cxxrt.cc). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <wchar.h>
#include <wctype.h>
#include <time.h>
#include <locale.h>

/* glibc's standard streams are variables */
#undef stdin
#undef stdout
#undef stderr
extern FILE __sf[3];
FILE *stdin = &__sf[0], *stdout = &__sf[1], *stderr = &__sf[2];

char __libc_single_threaded = 1;

/* --- ctype, glibc layout: index -128..255, bit n = 1 << n (big-endian) */
enum { U = 1 << 0, L = 1 << 1, A = 1 << 2, D = 1 << 3, X = 1 << 4, S = 1 << 5,
       P = 1 << 6, G = 1 << 7, B = 1 << 8, C = 1 << 9, N = 1 << 10, AN = 1 << 11 };
static unsigned short ct_b[384];
static int ct_lo[384], ct_up[384];
static const unsigned short *ct_bp = ct_b + 128;
static const int *ct_lop = ct_lo + 128, *ct_upp = ct_up + 128;
static void ct_init(void)
{
    static int done;
    if (done) return;
    done = 1;
    for (int i = -128; i < 256; i++) {
        int c = i & 255;
        unsigned short m = 0;
        if (c < 128) {
            if (c >= 'A' && c <= 'Z') m |= U | A | AN | P | G;
            if (c >= 'a' && c <= 'z') m |= L | A | AN | P | G;
            if (c >= '0' && c <= '9') m |= D | X | AN | P | G;
            if ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) m |= X;
            if (c == ' ' || (c >= 9 && c <= 13)) m |= S;
            if (c == ' ' || c == '\t') m |= B;
            if (c < 32 || c == 127) m |= C;
            if (c == ' ') m |= P;
            if (c > 32 && c < 127 && !(m & AN)) m |= N | P | G;
        }
        ct_b[i + 128] = m;
        ct_lo[i + 128] = (c >= 'A' && c <= 'Z') ? c + 32 : i;
        ct_up[i + 128] = (c >= 'a' && c <= 'z') ? c - 32 : i;
    }
}
const unsigned short **__ctype_b_loc(void) { ct_init(); return &ct_bp; }
const int **__ctype_tolower_loc(void) { ct_init(); return &ct_lop; }
const int **__ctype_toupper_loc(void) { ct_init(); return &ct_upp; }
size_t __ctype_get_mb_cur_max(void) { return 1; }

/* --- the "C" locale, as glibc lays it out */
struct glibc_locale {
    void *locales[13];
    const unsigned short *ctype_b;
    const int *ctype_tolower, *ctype_toupper;
    const char *names[13];
};
static struct glibc_locale c_loc;
static int c_name(const char *n) { return !n || !*n || !strcmp(n, "C") || !strcmp(n, "POSIX"); }
void *__newlocale(int mask, const char *name, void *base)
{
    (void)mask; (void)base;
    if (!c_name(name)) { errno = ENOENT; return 0; }
    ct_init();
    c_loc.ctype_b = ct_bp; c_loc.ctype_tolower = ct_lop; c_loc.ctype_toupper = ct_upp;
    for (int i = 0; i < 13; i++) c_loc.names[i] = "C";
    return &c_loc;
}
void *__duplocale(void *l) { return l; }
void __freelocale(void *l) { (void)l; }
void *__uselocale(void *l) { (void)l; return &c_loc; }
char *__nl_langinfo_l(int item, void *l) { (void)item; (void)l; return (char *)""; }

wctype_t __wctype_l(const char *n, void *l) { (void)l; return wctype(n); }
int __iswctype_l(wint_t c, wctype_t t, void *l) { (void)l; return iswctype(c, t); }
wint_t __towupper_l(wint_t c, void *l) { (void)l; return towupper(c); }
wint_t __towlower_l(wint_t c, void *l) { (void)l; return towlower(c); }
int __strcoll_l(const char *a, const char *b, void *l) { (void)l; return strcoll(a, b); }
size_t __strxfrm_l(char *d, const char *s, size_t n, void *l) { (void)l; return strxfrm(d, s, n); }
int __wcscoll_l(const wchar_t *a, const wchar_t *b, void *l) { (void)l; return wcscoll(a, b); }
size_t __wcsxfrm_l(wchar_t *d, const wchar_t *s, size_t n, void *l) { (void)l; return wcsxfrm(d, s, n); }
float __strtof_l(const char *s, char **e, void *l) { (void)l; return strtof(s, e); }
double __strtod_l(const char *s, char **e, void *l) { (void)l; return strtod(s, e); }
long double __strtold_l(const char *s, char **e, void *l) { (void)l; return strtold(s, e); }
size_t __strftime_l(char *s, size_t n, const char *f, const struct tm *t, void *l)
{ (void)l; return strftime(s, n, f, t); }
size_t __wcsftime_l(wchar_t *s, size_t n, const wchar_t *f, const struct tm *t, void *l)
{ (void)l; return wcsftime(s, n, f, t); }

/* --- _FORTIFY_SOURCE entry points */
size_t __mbsrtowcs_chk(wchar_t *d, const char **s, size_t n, mbstate_t *ps, size_t dl)
{ (void)dl; return mbsrtowcs(d, s, n, ps); }
size_t __mbsnrtowcs_chk(wchar_t *d, const char **s, size_t nms, size_t n, mbstate_t *ps, size_t dl)
{ (void)dl; return mbsnrtowcs(d, s, nms, n, ps); }
wchar_t *__wmemset_chk(wchar_t *s, wchar_t c, size_t n, size_t dl) { (void)dl; return wmemset(s, c, n); }

/* --- large-file names */
int fseeko64(FILE *f, long long o, int w) { return fseeko(f, (off_t)o, w); }
long long ftello64(FILE *f) { return ftello(f); }

/* --- no message catalogues, no iconv */
char *gettext(const char *m) { return (char *)m; }
char *dgettext(const char *d, const char *m) { (void)d; return (char *)m; }
char *bind_textdomain_codeset(const char *d, const char *c) { (void)d; (void)c; return 0; }
void *iconv_open(const char *t, const char *f) { (void)t; (void)f; errno = EINVAL; return (void *)-1; }
size_t iconv(void *cd, char **i, size_t *il, char **o, size_t *ol)
{ (void)cd; (void)i; (void)il; (void)o; (void)ol; errno = EBADF; return (size_t)-1; }
int iconv_close(void *cd) { (void)cd; return 0; }

/* --- one thread */
int pthread_mutex_lock(void *m) { (void)m; return 0; }
int pthread_mutex_unlock(void *m) { (void)m; return 0; }
int pthread_once(int *once, void (*init)(void))
{
    if (*once == 0) { *once = 2; init(); }
    return 0;
}

/* C11 timespec_get (libstdc++'s <ctime> names it) */
int timespec_get(struct timespec *ts, int base)
{
    if (base != 1) return 0;
    ts->tv_sec = time(0); ts->tv_nsec = 0;
    return base;
}
