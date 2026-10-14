/* dlfcn.h -- CMS: the native libraries are linked in; dlsym looks the
   symbol up in a table made at build time (VM/370plus 9.35). */
#ifndef CMS_DLFCN_H
#define CMS_DLFCN_H
#define RTLD_LAZY 1
#define RTLD_NOW 2
#define RTLD_GLOBAL 0x100
#define RTLD_LOCAL 0
void *dlopen(const char *, int);
void *dlsym(void *, const char *);
int dlclose(void *);
char *dlerror(void);
#endif
