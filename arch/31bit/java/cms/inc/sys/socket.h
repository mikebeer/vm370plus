/* sys/socket.h for CMS: no sockets (VM/370plus 9.35); the calls fail ENOSYS */
#ifndef _CMS_SYS_SOCKET_H
#define _CMS_SYS_SOCKET_H
#include <sys/types.h>
#include <sys/uio.h>
typedef unsigned int socklen_t;
typedef unsigned short sa_family_t;
struct sockaddr { sa_family_t sa_family; char sa_data[14]; };
struct sockaddr_storage { sa_family_t ss_family; char __ss[126]; };
#define AF_UNSPEC 0
#define AF_UNIX 1
#define AF_INET 2
#define AF_INET6 10
#define PF_INET AF_INET
#define PF_INET6 AF_INET6
#define SOCK_STREAM 1
#define SOCK_DGRAM 2
#define SOL_SOCKET 1
#define SO_ERROR 4
#define SO_TYPE 3
#define SO_REUSEADDR 2
#define SO_KEEPALIVE 9
#define SO_RCVTIMEO 20
#define SO_SNDTIMEO 21
#define SO_RCVBUF 8
#define SO_SNDBUF 7
#define SO_LINGER 13
#define SO_BROADCAST 6
#define SO_OOBINLINE 10
#define MSG_PEEK 2
#define SHUT_RD 0
#define SHUT_WR 1
#define SHUT_RDWR 2
int socket(int, int, int);
int connect(int, const struct sockaddr *, socklen_t);
int bind(int, const struct sockaddr *, socklen_t);
int listen(int, int);
int accept(int, struct sockaddr *, socklen_t *);
int getsockname(int, struct sockaddr *, socklen_t *);
int getpeername(int, struct sockaddr *, socklen_t *);
int getsockopt(int, int, int, void *, socklen_t *);
int setsockopt(int, int, int, const void *, socklen_t);
ssize_t send(int, const void *, size_t, int);
ssize_t recv(int, void *, size_t, int);
ssize_t sendto(int, const void *, size_t, int, const struct sockaddr *, socklen_t);
ssize_t recvfrom(int, void *, size_t, int, struct sockaddr *, socklen_t *);
int shutdown(int, int);
#endif
