/* cobcms.c -- POSIX names libcob needs on CMS, mapped onto the M5f runtime
   (cmsrt.c provides _open/_read/_lseek/_close/_stat over CMS files). */
#include <stdarg.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
int _open(const char *, int, int);
int _close(int);
int _read(int, char *, int);
int _lseek(int, int, int);
int _stat(const char *, struct stat *);
int _getpid(void);
#include <string.h>
/* libcob opens SEQUENTIAL, RELATIVE and SORT files with open(): those are
   record images, so they are CMS files in binary mode ("(b" for cmsrt);
   LINE SEQUENTIAL goes through fopen() and stays text (ASCII <-> EBCDIC). */
int open(const char *name, int flags, ...)
{
	va_list ap; int mode; char nb[256];
	va_start(ap, flags); mode = va_arg(ap, int); va_end(ap);
	if (strlen(name) < sizeof nb - 3 && !strchr(name, '(')) {
		strcpy(nb, name); strcat(nb, "(b");
		return _open(nb, flags, mode);
	}
	return _open(name, flags, mode);
}
int close(int fd) { return _close(fd); }
ssize_t read(int fd, void *p, size_t n) { return _read(fd, p, (int)n); }
off_t lseek(int fd, off_t off, int whence) { return _lseek(fd, (int)off, whence); }
int fsync(int fd) { (void)fd; return 0; }
int getpid(void) { return _getpid(); }
char *getlogin(void) { return "CMSUSER"; }
int access(const char *name, int mode)
{
	struct stat st; (void)mode;
	if (_stat(name, &st) != 0) { errno = ENOENT; return -1; }
	return 0;
}
int _unlink(const char *);
int unlink(const char *name) { return _unlink(name); }
