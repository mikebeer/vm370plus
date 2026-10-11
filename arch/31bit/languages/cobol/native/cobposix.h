/* cobposix.h -- what libcob and GMP need beyond GCCLIB31 (ANSI C on CMS),
   for GnuCOBOL compiled natively by GCC380 (9.19).  Included from the
   native config.h, after GCCLIB31's stdio/stdlib/string, so the fixes
   below win.  Implementations: cobposix.c.  Everything runs in EBCDIC. */
#ifndef COBPOSIX_H
#define COBPOSIX_H
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

/* GCCLIB31's getc(s) macro expands to fgetc((c)) */
#undef getc
#define getc(s) fgetc(s)
#undef putc
#define putc(c, s) fputc((c), (s))

/* Unix file names ("dir/fn.ft", "fn.ft.fm") to CMS file ids ("FN FT FM"):
   every name libcob passes to stdio goes through these */
FILE *cob_fopen(const char *name, const char *mode);
int cob_remove(const char *name);
int cob_rename(const char *from, const char *to);
#define fopen(n, m) cob_fopen((n), (m))
#define remove(n) cob_remove(n)
#define rename(a, b) cob_rename((a), (b))
void cob_cmsname(const char *name, char *out, int forread);

/* file descriptors: whole files in storage, written back at close */
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2
#define O_ACCMODE 3
#define O_CREAT 0100
#define O_TRUNC 01000
#define O_APPEND 02000
#define O_EXCL 0200
#define O_BINARY 0
#define F_OK 0
#define R_OK 4
#define W_OK 2
#define X_OK 1
int open(const char *name, int flags, ...);
int close(int fd);
int read(int fd, void *p, unsigned int n);
int write(int fd, const void *p, unsigned int n);
long lseek(int fd, long off, int whence);
int fsync(int fd);
int access(const char *name, int mode);
int unlink(const char *name);
int isatty(int fd);
int fileno(FILE *f);
FILE *fdopen(int fd, const char *mode);
int getpid(void);
char *getlogin(void);
char *getcwd(char *buf, size_t n);
int chdir(const char *dir);
int mkdir(const char *dir, int mode);
int rmdir(const char *dir);
unsigned int sleep(unsigned int s);
int setenv(const char *n, const char *v, int overwrite);
int unsetenv(const char *n);
int putenv(char *s);
char *cob_getenv(const char *n);
#define getenv(n) cob_getenv(n)
int strcasecmp(const char *a, const char *b);
int strncasecmp(const char *a, const char *b, size_t n);
char *strdup(const char *s);
int snprintf(char *buf, size_t n, const char *fmt, ...);
#include <stdarg.h>
int vsnprintf(char *buf, size_t n, const char *fmt, va_list ap);
long long strtoll(const char *s, char **end, int base);
#endif
