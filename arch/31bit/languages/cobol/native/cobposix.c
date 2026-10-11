/* cobposix.c -- POSIX layer for GnuCOBOL compiled natively on CMS by
   GCC380 with GCCLIB31 (9.19).  See cobposix.h.
   File names: "dir/fn.ft", "fn.ft.fm" or "FN FT FM"; "x/" is file mode x,
   "./" or no mode means: reading -- any accessed disk, writing -- A.
   File descriptors hold the whole file in storage and write it back at
   close (as M5f's cmsrt does); records are concatenated (binary). */
#include "cobposix.h"
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <cmssys.h>
#include <cmsruntm.h>

#define NFD 32
#define RECMAX 32760

struct cfd {
	int	used, writing;
	char	fid[18];
	char	*data;
	long	len, cap, pos;
};
static struct cfd fds[NFD];

/* -------------------------------------------------------------- names */
static void put8(char *d, const char *s, int n)
{
	int i;
	for (i = 0; i < n; i++) {
		d[i] = *s ? (char)toupper((unsigned char)*s++) : ' ';
	}
}

/* fid: 18 chars, FN(8) FT(8) FM(2); returns 0, or -1 for a bad name */
static int unix2fid(const char *name, char *fid, int forread)
{
	char part[3][9];
	char mode[3];
	const char *p, *slash;
	int np = 0, k = 0;

	memset(part, 0, sizeof part);
	mode[0] = mode[1] = mode[2] = 0;
	while (*name == ' ') name++;
	if (strchr(name, ' ')) {			/* already "FN FT FM" */
		p = name;
		while (*p && np < 3) {
			k = 0;
			while (*p && *p != ' ') { if (k < 8) part[np][k++] = *p; p++; }
			np++;
			while (*p == ' ') p++;
		}
	} else {
		slash = strrchr(name, '/');
		if (slash) {
			if (!(slash - name == 1 && name[0] == '.') && slash > name) {
				mode[0] = name[0];
				if (slash - name > 1 && isdigit((unsigned char)name[1])) mode[1] = name[1];
			}
			name = slash + 1;
		}
		for (p = name; *p; p++) {
			if (*p == '.') {
				if (k) { np++; k = 0; }
				if (np == 3) break;
				continue;
			}
			if (np >= 3) break;
			if (k < 8) part[np][k++] = *p;
		}
		if (k) np++;
	}
	if (np < 1) return -1;
	if (np < 2) strcpy(part[1], "DAT");	/* a bare name: FN DAT */
	put8(fid, part[0], 8);
	put8(fid + 8, part[1], 8);
	if (np >= 3) {
		fid[16] = (char)toupper((unsigned char)part[2][0]);
		fid[17] = part[2][1] ? part[2][1] : '1';
	} else if (mode[0]) {
		fid[16] = (char)toupper((unsigned char)mode[0]);
		fid[17] = mode[1] ? mode[1] : '1';
	} else if (forread) {
		fid[16] = '*'; fid[17] = ' ';
	} else {
		fid[16] = 'A'; fid[17] = '1';
	}
	return 0;
}

/* resolve FM '*' to the disk the file is on; 0 if it exists */
static int fidstate(char *fid, CMSFILEINFO **info)
{
	CMSFILEINFO *fi = NULL;
	char f[19];
	int rc;
	memcpy(f, fid, 18); f[18] = 0;
	rc = CMSfileState(f, &fi);
	if (rc == 0 && fi) {
		if (fid[16] == '*') {
			fid[16] = fi->filemode[0];
			fid[17] = fi->filemode[1];
		}
		if (info) *info = fi;
	}
	return rc;
}

void cob_cmsname(const char *name, char *out, int forread)
{
	char fid[18];
	if (unix2fid(name, fid, forread)) { strcpy(out, name); return; }
	if (forread && fid[16] == '*' && fidstate(fid, NULL) != 0) {
		fid[16] = 'A'; fid[17] = '1';
	}
	memcpy(out, fid, 8); out[8] = ' ';
	memcpy(out + 9, fid + 8, 8); out[17] = ' ';
	out[18] = fid[16]; out[19] = fid[17] == ' ' ? 0 : fid[17]; out[20] = 0;
}

#undef fopen
#undef remove
#undef rename
FILE *cob_fopen(const char *name, const char *mode)
{
	char cmsid[48];
	int rd = mode[0] == 'r' && !strchr(mode, '+');
	if (!strcmp(name, "CONSOLE") || !strcmp(name, "PRINTER") ||
	    !strcmp(name, "PUNCH") || !strcmp(name, "READER")) {
		return fopen(name, mode);
	}
	cob_cmsname(name, cmsid, rd || mode[0] == 'a' || mode[0] == 'r');
	if (!rd) strcat(cmsid, " V 32760");
	return fopen(cmsid, mode);
}

int cob_remove(const char *name)
{
	char fid[18], f[19];
	if (unix2fid(name, fid, 1)) { errno = EINVAL; return -1; }
	if (fid[16] == '*' && fidstate(fid, NULL)) { errno = ENOENT; return -1; }
	memcpy(f, fid, 18); f[18] = 0;
	if (CMSfileErase(f)) { errno = ENOENT; return -1; }
	return 0;
}

int unlink(const char *name) { return cob_remove(name); }

int cob_rename(const char *from, const char *to)
{
	char a[18], b[18], fa[19], fb[19];
	if (unix2fid(from, a, 1) || unix2fid(to, b, 0)) { errno = EINVAL; return -1; }
	if (a[16] == '*' && fidstate(a, NULL)) { errno = ENOENT; return -1; }
	b[16] = a[16]; b[17] = a[17];
	memcpy(fb, b, 18); fb[18] = 0;
	CMSfileErase(fb);			/* rename() replaces */
	memcpy(fa, a, 18); fa[18] = 0;
	if (CMSfileRename(fa, fb)) { errno = EACCES; return -1; }
	return 0;
}

/* ---------------------------------------------------- whole-file I/O */
static int grow(struct cfd *f, long more)
{
	long n;
	char *d;
	if (f->len + more <= f->cap) return 0;
	n = f->cap ? f->cap * 2 : 4096;
	while (n < f->len + more) n *= 2;
	d = realloc(f->data, n);
	if (!d) return -1;
	f->data = d; f->cap = n;
	return 0;
}

static int read_whole(struct cfd *f)
{
	CMSFILE *fs = _lmalloc(sizeof(CMSFILE));
	char *buf = _lmalloc(RECMAX);
	char fid[19];
	int rc, n;
	if (!fs || !buf) return ENOMEM;
	memcpy(fid, f->fid, 18); fid[18] = 0;
	rc = CMSfileOpen(fid, buf, RECMAX, 'V', 1, 1, fs);
	if (rc == 0) {
		for (;;) {
			n = 0;
			rc = CMSfileRead(fs, 0, &n);
			if (rc == 12) { rc = 0; break; }
			if (rc) break;
			if (grow(f, n)) { rc = ENOMEM; break; }
			memcpy(f->data + f->len, buf, n);
			f->len += n;
		}
		CMSfileClose(fs);
	}
	_lfree(buf); _lfree(fs);
	return rc == 0 ? 0 : (rc == 28 || rc == 1 ? ENOENT : EIO);
}

static int write_whole(struct cfd *f)
{
	CMSFILE *fs;
	char *buf;
	char fid[19];
	long i = 0;
	int rc = 0, n;
	memcpy(fid, f->fid, 18); fid[18] = 0;
	CMSfileErase(fid);
	if (f->len == 0) return 0;		/* CMS has no empty files */
	fs = _lmalloc(sizeof(CMSFILE));
	buf = _lmalloc(RECMAX);
	if (!fs || !buf) return ENOMEM;
	rc = CMSfileOpen(fid, buf, RECMAX, 'V', 1, 0, fs);
	if (rc == 28) rc = 0;			/* new file */
	while (!rc && i < f->len) {
		n = f->len - i > RECMAX ? RECMAX : (int)(f->len - i);
		memcpy(buf, f->data + i, n);
		rc = CMSfileWrite(fs, 0, n);
		i += n;
	}
	CMSfileClose(fs);
	_lfree(buf); _lfree(fs);
	return rc ? EIO : 0;
}

int open(const char *name, int flags, ...)
{
	int fd, rc;
	struct cfd *f;
	for (fd = 3; fd < NFD && fds[fd].used; fd++) ;
	if (fd == NFD) { errno = EMFILE; return -1; }
	f = &fds[fd];
	memset(f, 0, sizeof *f);
	if (unix2fid(name, f->fid, 1)) { errno = EINVAL; return -1; }
	f->used = 1;
	if ((flags & O_ACCMODE) == O_RDONLY) {
		if (f->fid[16] == '*' && fidstate(f->fid, NULL)) {
			f->used = 0; errno = ENOENT; return -1;
		}
		rc = read_whole(f);
		if (rc) { free(f->data); f->used = 0; errno = rc; return -1; }
		return fd;
	}
	f->writing = 1;
	if (f->fid[16] == '*') {
		if (fidstate(f->fid, NULL)) { f->fid[16] = 'A'; f->fid[17] = '1'; }
	}
	if (!(flags & O_TRUNC)) {		/* OPEN I-O, EXTEND: keep the data */
		if (read_whole(f) == 0) {
			f->pos = (flags & O_APPEND) ? f->len : 0;
		} else {
			f->len = 0;
		}
	}
	return fd;
}

int close(int fd)
{
	struct cfd *f;
	int rc = 0;
	if (fd < 3) return 0;
	if (fd >= NFD || !fds[fd].used) { errno = EBADF; return -1; }
	f = &fds[fd];
	if (f->writing) rc = write_whole(f);
	free(f->data);
	f->used = 0;
	if (rc) { errno = rc; return -1; }
	return 0;
}

int write(int fd, const void *p, unsigned int n)
{
	struct cfd *f;
	if (fd == 1 || fd == 2) {
		fwrite(p, 1, n, fd == 1 ? stdout : stderr);
		return n;
	}
	if (fd < 3 || fd >= NFD || !fds[fd].used || !fds[fd].writing) { errno = EBADF; return -1; }
	f = &fds[fd];
	if (f->pos + (long)n > f->len && grow(f, f->pos + n - f->len)) { errno = ENOSPC; return -1; }
	memcpy(f->data + f->pos, p, n);
	f->pos += n;
	if (f->pos > f->len) f->len = f->pos;
	return n;
}

int read(int fd, void *p, unsigned int n)
{
	struct cfd *f;
	long k;
	if (fd == 0) return fread(p, 1, n, stdin);
	if (fd < 3 || fd >= NFD || !fds[fd].used) { errno = EBADF; return -1; }
	f = &fds[fd];
	k = f->len - f->pos;
	if ((long)n < k) k = n;
	if (k < 0) k = 0;
	memcpy(p, f->data + f->pos, k);
	f->pos += k;
	return (int)k;
}

long lseek(int fd, long off, int whence)
{
	struct cfd *f;
	long np;
	if (fd < 3) return 0;
	if (fd >= NFD || !fds[fd].used) { errno = EBADF; return -1; }
	f = &fds[fd];
	np = whence == SEEK_SET ? off : whence == SEEK_CUR ? f->pos + off : f->len + off;
	if (np < 0) { errno = EINVAL; return -1; }
	if (np > f->len && f->writing) {
		if (grow(f, np - f->len)) { errno = ENOSPC; return -1; }
		memset(f->data + f->len, 0, np - f->len);
		f->len = np;
	}
	if (np > f->len) np = f->len;
	f->pos = np;
	return np;
}

int fsync(int fd) { (void)fd; return 0; }
int isatty(int fd) { return fd < 3; }
int fileno(FILE *f) { return f == stdin ? 0 : f == stdout ? 1 : f == stderr ? 2 : 3; }
FILE *fdopen(int fd, const char *mode) { (void)fd; (void)mode; errno = EINVAL; return NULL; }

int fstat(int fd, struct stat *st)
{
	memset(st, 0, sizeof *st);
	if (fd < 3) { st->st_mode = S_IFCHR; return 0; }
	if (fd >= NFD || !fds[fd].used) { errno = EBADF; return -1; }
	st->st_mode = S_IFREG | 0666;
	st->st_size = fds[fd].len;
	st->st_blksize = 4096;
	return 0;
}

int stat(const char *name, struct stat *st)
{
	char fid[18];
	CMSFILEINFO *fi = NULL;
	memset(st, 0, sizeof *st);
	if (!strcmp(name, ".") || !strcmp(name, "./") || !strcmp(name, "/")) {
		st->st_mode = S_IFDIR | 0777;
		return 0;
	}
	if (unix2fid(name, fid, 1) || fidstate(fid, &fi)) { errno = ENOENT; return -1; }
	st->st_mode = S_IFREG | 0666;
	if (fi) st->st_size = (long)fi->numRecords * fi->lrecl;
	st->st_blksize = 4096;
	st->st_mtime = time(NULL);
	return 0;
}

int access(const char *name, int mode)
{
	struct stat st;
	(void)mode;
	return stat(name, &st);
}

/* ----------------------------------------------------------- process */
int getpid(void) { return 1; }
char *getlogin(void) { return "CMSUSER"; }
char *getcwd(char *buf, size_t n)
{
	if (!buf || n < 2) { errno = ERANGE; return NULL; }
	strcpy(buf, ".");
	return buf;
}
int chdir(const char *dir) { (void)dir; errno = ENOENT; return -1; }
int mkdir(const char *dir, int mode) { (void)dir; (void)mode; errno = EACCES; return -1; }
int rmdir(const char *dir) { (void)dir; errno = EACCES; return -1; }
unsigned int sleep(unsigned int s)
{
	char cmd[40];
	sprintf(cmd, "CP SLEEP %u SEC", s);
	system(cmd);
	return 0;
}

/* ------------------------------------------------------- environment */
#define NENV 64
static char *envs[NENV];

static int envfind(const char *n)
{
	size_t l = strlen(n);
	int i;
	for (i = 0; i < NENV; i++) {
		if (envs[i] && !strncmp(envs[i], n, l) && envs[i][l] == '=') return i;
	}
	return -1;
}

int setenv(const char *n, const char *v, int overwrite)
{
	int i = envfind(n);
	char *s;
	if (i >= 0 && !overwrite) return 0;
	s = malloc(strlen(n) + strlen(v) + 2);
	if (!s) { errno = ENOMEM; return -1; }
	sprintf(s, "%s=%s", n, v);
	if (i < 0) {
		for (i = 0; i < NENV && envs[i]; i++) ;
		if (i == NENV) { free(s); errno = ENOMEM; return -1; }
	} else {
		free(envs[i]);
	}
	envs[i] = s;
	return 0;
}

int unsetenv(const char *n)
{
	int i = envfind(n);
	if (i >= 0) { free(envs[i]); envs[i] = NULL; }
	return 0;
}

int putenv(char *s)
{
	char *eq = strchr(s, '=');
	char n[256];
	if (!eq || eq - s >= (int)sizeof n) { errno = EINVAL; return -1; }
	memcpy(n, s, eq - s); n[eq - s] = 0;
	return setenv(n, eq + 1, 1);
}

#undef getenv
char *cob_getenv(const char *n)
{
	int i = envfind(n);
	if (i >= 0) return strchr(envs[i], '=') + 1;
	return getenv(n);
}

/* ------------------------------------------------------------ strings */
int strcasecmp(const char *a, const char *b)
{
	int ca, cb;
	do {
		ca = toupper((unsigned char)*a++);
		cb = toupper((unsigned char)*b++);
	} while (ca && ca == cb);
	return ca - cb;
}

int strncasecmp(const char *a, const char *b, size_t n)
{
	int ca = 0, cb = 0;
	while (n--) {
		ca = toupper((unsigned char)*a++);
		cb = toupper((unsigned char)*b++);
		if (!ca || ca != cb) break;
	}
	return ca - cb;
}

char *strdup(const char *s)
{
	char *d = malloc(strlen(s) + 1);
	if (d) strcpy(d, s);
	return d;
}

/* vsnprintf: GCCLIB31 has vsprintf only; format into a large work
   buffer, then copy what fits */
#define SNBUF 65536
static char *snbuf;

int vsnprintf(char *buf, size_t n, const char *fmt, va_list ap)
{
	int len;
	if (!snbuf && !(snbuf = malloc(SNBUF))) return -1;
	len = vsprintf(snbuf, fmt, ap);
	if (len < 0) return len;
	if (n > 0) {
		size_t c = (size_t)len < n - 1 ? (size_t)len : n - 1;
		memcpy(buf, snbuf, c);
		buf[c] = 0;
	}
	return len;
}

int snprintf(char *buf, size_t n, const char *fmt, ...)
{
	va_list ap;
	int len;
	va_start(ap, fmt);
	len = vsnprintf(buf, n, fmt, ap);
	va_end(ap);
	return len;
}

long long strtoll(const char *s, char **end, int base)
{
	unsigned long long v = 0;
	int neg = 0, d;
	const char *p = s;
	while (isspace((unsigned char)*p)) p++;
	if (*p == '-' || *p == '+') neg = *p++ == '-';
	if ((base == 0 || base == 16) && p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
		p += 2; base = 16;
	} else if (base == 0) {
		base = p[0] == '0' ? 8 : 10;
	}
	for (;; p++) {
		if (isdigit((unsigned char)*p)) d = *p - '0';
		else if (isalpha((unsigned char)*p)) d = toupper((unsigned char)*p) - 'A' + 10;
		else break;
		if (d >= base) break;
		v = v * base + d;
	}
	if (end) *end = (char *)p;
	return neg ? -(long long)v : (long long)v;
}

/* ------------------------------------------- no processes, no modules */
#include "dlfcn.h"
static char dlmsg[] = "dynamic loading is not available on CMS";
void *dlopen(const char *file, int mode) { (void)file; (void)mode; return NULL; }
void *dlsym(void *h, const char *n) { (void)h; (void)n; return NULL; }
int dlclose(void *h) { (void)h; return 0; }
char *dlerror(void) { return dlmsg; }
int fork(void) { errno = ENOSYS; return -1; }
int kill(int pid, int sig) { (void)pid; (void)sig; errno = EPERM; return -1; }

/* System/370 hexadecimal floating point has no infinities or NaNs */
int finite(double d) { (void)d; return 1; }
int isnan(double d) { (void)d; return 0; }
