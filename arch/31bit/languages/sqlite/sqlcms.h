/* sqlcms.h -- included first (-include) when SQLite is built for CMS
   (VM/370plus 9.4): what newlib's headers leave undeclared and cmsrt has */
#include <sys/types.h>
#include <sys/stat.h>
int lstat(const char *path, struct stat *st);
#include <time.h>
int nanosleep(const struct timespec *req, struct timespec *rem);
/* the shell looks for ~/.sqliterc: HOME is "." on CMS (no such file, no
   warning) */
#include <stdlib.h>
#include <string.h>
static char *sqlcms_getenv(const char *n)
{
    char *v = getenv(n);
    return v || strcmp(n, "HOME") ? v : (char *)".";
}
#define getenv(n) sqlcms_getenv(n)
