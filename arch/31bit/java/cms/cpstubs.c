/* cpstubs.c -- CMS replacements for Classpath natives that need a terminal,
   a time zone database or sockets (VM/370plus 9.35). */
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <jni.h>

JNIEXPORT jstring JNICALL
Java_java_io_VMConsole_readPassword(JNIEnv *env, jclass c, jobject con)
{ return NULL; }

JNIEXPORT jstring JNICALL
Java_java_util_VMTimeZone_getSystemTimeZoneId(JNIEnv *env, jclass c)
{
    const char *tz = getenv("TZ");
    return (*env)->NewStringUTF(env, tz && *tz ? tz : "GMT");
}

#define NOSOCK { errno = ENOSYS; return -1; }
int socket(int a, int b, int c) NOSOCK
int connect(int s, const struct sockaddr *a, socklen_t l) NOSOCK
int bind(int s, const struct sockaddr *a, socklen_t l) NOSOCK
int listen(int s, int n) NOSOCK
int accept(int s, struct sockaddr *a, socklen_t *l) NOSOCK
int getsockname(int s, struct sockaddr *a, socklen_t *l) NOSOCK
int getpeername(int s, struct sockaddr *a, socklen_t *l) NOSOCK
int getsockopt(int s, int a, int b, void *v, socklen_t *l) NOSOCK
int setsockopt(int s, int a, int b, const void *v, socklen_t l) NOSOCK
ssize_t send(int s, const void *b, size_t n, int f) NOSOCK
ssize_t recv(int s, void *b, size_t n, int f) NOSOCK
ssize_t sendto(int s, const void *b, size_t n, int f, const struct sockaddr *a, socklen_t l) NOSOCK
ssize_t recvfrom(int s, void *b, size_t n, int f, struct sockaddr *a, socklen_t *l) NOSOCK
int shutdown(int s, int h) NOSOCK

ssize_t readv(int fd, const struct iovec *v, int n)
{
    ssize_t t = 0, r;
    int i;
    for (i = 0; i < n; i++) {
        r = read(fd, v[i].iov_base, v[i].iov_len);
        if (r < 0) return t ? t : -1;
        t += r;
        if ((size_t)r < v[i].iov_len) break;
    }
    return t;
}

ssize_t writev(int fd, const struct iovec *v, int n)
{
    ssize_t t = 0, r;
    int i;
    for (i = 0; i < n; i++) {
        r = write(fd, v[i].iov_base, v[i].iov_len);
        if (r < 0) return t ? t : -1;
        t += r;
        if ((size_t)r < v[i].iov_len) break;
    }
    return t;
}

#include <sys/select.h>
int select(int n, fd_set *r, fd_set *w, fd_set *e, struct timeval *t)
{ errno = ENOSYS; return -1; }
