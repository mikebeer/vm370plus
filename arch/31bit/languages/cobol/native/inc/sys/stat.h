/* sys/stat.h for GnuCOBOL on CMS: a CMS file is a regular file; "." (the
   accessed disks) is the only directory */
#ifndef COB_SYS_STAT_H
#define COB_SYS_STAT_H
#include <sys/types.h>
#include <time.h>
struct stat {
	mode_t	st_mode;
	off_t	st_size;
	time_t	st_mtime;
	long	st_blksize;
	int	st_nlink;
	int	st_dev;
	int	st_ino;
};
#define S_IFMT  0170000
#define S_IFDIR 0040000
#define S_IFREG 0100000
#define S_IFCHR 0020000
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#define S_ISCHR(m) (((m) & S_IFMT) == S_IFCHR)
#define S_IRUSR 0400
#define S_IWUSR 0200
#define S_IRWXU 0700
#define S_IRGRP 040
#define S_IWGRP 020
#define S_IROTH 04
#define S_IWOTH 02
int stat(const char *name, struct stat *st);
int fstat(int fd, struct stat *st);
#endif
