/* sqlcms.h -- included first (-include) when SQLite is built for CMS
   (VM/370plus 9.4): what newlib's headers leave undeclared and cmsrt has */
#include <sys/types.h>
#include <sys/stat.h>
int lstat(const char *path, struct stat *st);
#include <time.h>
int nanosleep(const struct timespec *req, struct timespec *rem);
