/* VM/370plus CMS: what Regina needs that newlib + cmsrt do not have */
#include <stddef.h>
#include <sys/types.h>
struct group;
struct group *getgrgid(gid_t gid) { (void)gid; return NULL; }
