/* cmsdl.c -- dlopen stubs for CMS: nothing can be loaded dynamically. */
#include <stddef.h>
#include "dlfcn.h"
static char msg[] = "dynamic loading is not available on CMS";
void *dlopen(const char *file, int mode) { (void)file; (void)mode; return NULL; }
void *dlsym(void *h, const char *n) { (void)h; (void)n; return NULL; }
int dlclose(void *h) { (void)h; return 0; }
char *dlerror(void) { return msg; }
