# JamVM + GNU Classpath on CMS (VM/370plus 9.35) -- work in progress

JamVM 2.0 (with ../jamvm-s390.patch and jamvm-cms.patch) and the GNU
Classpath 0.99 natives, built with GCC 13 `-m31 -msoft-float` against newlib
and m5f/cmsrt.c, linked by m5f/xlcc.sh into JAMVM XIMAGE (runs under the XL
loader above 16 MB).

* `cmsthread.c` -- CMS has one task: POSIX threads are green threads
  (setjmp/longjmp contexts, stacks from the heap), switched only inside
  mutex/condition waits, sleep, sched_yield, sigsuspend, sigwait and join.
  pthread_kill marks a signal pending; the handler runs when the target
  thread next runs (JamVM's GC suspend: handler -> sigsuspend).  Also mmap
  (a read-only file mapping takes the file's buffer: `cms_fd_take`), uname,
  scandir (no extension jars).
* `os_cms.c` -- replaces os/linux/os.c; no libffi, so `callJNIMethod` is
  only a fallback.
* `gensigs.py` + JamVM's tools/gen-stubs.c -- a C stub for every native
  signature in glibj.zip and classes.zip (432 -> 138 shapes).
* `mkdl.sh` -- dlopen/dlsym over a table of every `Java_*` symbol linked in.
* `cpstubs.c` -- VMConsole, VMTimeZone, sockets (ENOSYS), readv/writev, select.
* `inc/`, `cpinc/config.h`, `config.h` -- headers newlib lacks, configs.
* Classpath natives used: classpath/jcl, jnilink; native-lib/cpio;
  java-lang (VMDouble, VMFloat, VMMath, VMSystem, reflect/VMArray);
  java-io (VMFile, VMObjectInputStream, VMObjectStreamClass);
  gnu/java/nio/VMChannel (includes javanio.c); java/nio/VMDirectByteBuffer;
  fdlibm (with `_mprec_log10` renamed: newlib has one); zlib 1.3.1.

On CMS the boot class path is `/jamvm.zip:/glibj.zip`, i.e. the CMS files
JAMVM ZIP and GLIBJ ZIP on any accessed disk.  Class files are CMS files
fn CLASS (binary).
