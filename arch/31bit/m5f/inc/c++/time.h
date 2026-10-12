/* time.h for C++ on VM/370plus: newlib's, plus timespec_get (C11),
   which libstdc++'s <ctime> names (cxxrt.cc has it) */
#include_next <time.h>
#ifndef TIME_UTC
#define TIME_UTC 1
#endif
#ifdef __cplusplus
extern "C"
#endif
int timespec_get(struct timespec *, int);
