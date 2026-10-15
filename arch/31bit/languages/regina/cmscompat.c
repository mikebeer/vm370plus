/* VM/370plus CMS: what Regina needs that newlib + cmsrt do not have */
#include <stddef.h>
#include <sys/types.h>
struct group;
struct group *getgrgid(gid_t gid) { (void)gid; return NULL; }
/* 9.39: Regina may run nested (an EXEC run by Regina calls another, under
   XLRES each in its own copy): a fixed heap per copy, not all of HIGHSTOR */
unsigned int cms_heap_mb = 16;
unsigned int cms_stack_mb = 2;
