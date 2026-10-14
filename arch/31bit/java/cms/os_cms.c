/* os_cms.c -- JamVM's OS layer for CMS (VM/370plus 9.35).  Replaces
   os/linux/os.c.  Native libraries are linked into the module; "loading"
   one looks its symbols up in a table built at link time (cmsdl.c). */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <dlfcn.h>
#include "jam.h"


long long nativePhysicalMemory() { return 64LL << 20; }

void *nativeStackBase() {
    pthread_attr_t attr; void *addr; size_t size;
    pthread_getattr_np(pthread_self(), &attr);
    pthread_attr_getstack(&attr, &addr, &size);
    return (char *)addr + size;
}

int nativeAvailableProcessors() { return 1; }
char *nativeLibError() { return dlerror(); }
char *nativeLibPath() { return getenv("LD_LIBRARY_PATH"); }
void *nativeLibOpen(char *path) { return dlopen(path, RTLD_LAZY); }
void nativeLibClose(void *handle) { dlclose(handle); }
void *nativeLibSym(void *handle, char *symbol) { return dlsym(handle, symbol); }

char *nativeLibMapName(char *name) {
    char *buff = sysMalloc(strlen(name) + sizeof("lib.so") + 1);
    sprintf(buff, "lib%s.so", name);
    return buff;
}

char *nativeJVMPath() {
    char *p = sysMalloc(sizeof "/usr/jamvm/lib/libjvm.so");
    strcpy(p, "/usr/jamvm/lib/libjvm.so");
    return p;
}

/* No libffi on CMS: every native signature in the class library has a
   generated stub (jni-stubs-cms.c), so this is reached only for a native
   method of an application class with an unusual signature. */
int nativeExtraArg(MethodBlock *mb) { return 0; }

uintptr_t *callJNIMethod(void *env, Class *class, char *sig, int num_args,
                         uintptr_t *ostack, unsigned char *func) {
    jam_fprintf(stderr, "JamVM/CMS: no JNI stub for signature %s\n", sig);
    exitVM(1);
    return NULL;
}
