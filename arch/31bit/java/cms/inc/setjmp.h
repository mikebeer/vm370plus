/* setjmp.h -- cmsrt's setjmp/longjmp (m5f) plus the sig* forms JamVM uses
   (CMS has no signal mask to save: they are the plain ones). */
#ifndef CMS_SETJMP_H
#define CMS_SETJMP_H
typedef int jmp_buf[16];
int setjmp(jmp_buf env);
void longjmp(jmp_buf env, int val) __attribute__((noreturn));
typedef jmp_buf sigjmp_buf;
#define sigsetjmp(env, save) setjmp(env)
#define siglongjmp(env, val) longjmp(env, val)
#endif
