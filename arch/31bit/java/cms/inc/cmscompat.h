/* cmscompat.h -- forced into every JamVM/Classpath compile for CMS */
#ifndef CMSCOMPAT_H
#define CMSCOMPAT_H
#include <signal.h>
#ifndef SA_RESTART
#define SA_RESTART 0x10000000
#endif
#ifndef SIGQUIT
#define SIGQUIT 3
#endif
#endif
