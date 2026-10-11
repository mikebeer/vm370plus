/* dlfcn.h for CMS (M5f): there is no dynamic loading; CALL resolves
   statically linked programs only (cobc -fstatic-call or COB_PRELOAD). */
#ifndef CMS_DLFCN_H
#define CMS_DLFCN_H
#define RTLD_LAZY 1
#define RTLD_NOW 2
#define RTLD_GLOBAL 0x100
#define RTLD_LOCAL 0
void *dlopen(const char *file, int mode);
void *dlsym(void *handle, const char *name);
int dlclose(void *handle);
char *dlerror(void);
#endif
