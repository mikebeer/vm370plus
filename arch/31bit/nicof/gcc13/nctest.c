/*
** NCTEST C -- VM/370plus 9.5: a test of NICOF's client for GCC 13
** (cmsrt).  NCTEST host port: gethostbyname, connect, 6000 bytes to an
** echo server and back (several 2 KB NICOF calls), select() with a timeout
** (the STIMER path), close.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "nicofclt.h"
#include "socket.h"

static char out[6000], in[6000];

int main(int argc, char *argv[])
{
  const char *host = argc > 1 ? argv[1] : "localhost";
  int port = argc > 2 ? atoi(argv[2]) : 8200, i, n, k, s;
  struct hostent *he;
  struct sockaddr_in a;
  fd_set rd;
  struct timeval tv;
  char lc[64];
  for (i = 0; host[i] && i < 63; i++)
    lc[i] = host[i] >= 'A' && host[i] <= 'Z' ? host[i] + 32 : host[i];
  lc[i] = 0;
  nicofclt_init();
  he = gethostbyname(lc);
  if (!he) { printf("NCTEST: gethostbyname(%s) failed, h_errno %d\n", lc, h_errno); return 8; }
  printf("NCTEST: %s is %s = %d.%d.%d.%d\n", lc, he->h_name,
         he->h_addr[0] & 255, he->h_addr[1] & 255, he->h_addr[2] & 255, he->h_addr[3] & 255);
  s = socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0) { printf("NCTEST: socket: %s\n", nicofsocket_errmsg(errno)); return 12; }
  a.sin_family = AF_INET;
  a.sin_port = htons(port);
  memcpy(&a.sin_addr.s_addr, he->h_addr, 4);
  if (connect(s, (struct sockaddr *)&a, sizeof a) < 0) {
    printf("NCTEST: connect: %s\n", nicofsocket_errmsg(errno)); return 16;
  }
  for (i = 0; i < (int)sizeof out; i++) out[i] = 'A' + i % 26;
  for (n = 0; n < (int)sizeof out; n += k) {
    k = sizeof out - n; if (k > 2000) k = 2000;
    k = send(s, out + n, k, 0);
    if (k <= 0) { printf("NCTEST: send: %s\n", nicofsocket_errmsg(errno)); return 20; }
  }
  for (n = 0; n < (int)sizeof in; n += k) {
    k = sizeof in - n; if (k > 2000) k = 2000;
    k = recv(s, in + n, k, 0);
    if (k <= 0) { printf("NCTEST: recv: %s\n", nicofsocket_errmsg(errno)); return 24; }
  }
  printf("NCTEST: %d bytes sent and echoed: %s\n", n,
         memcmp(in, out, sizeof in) ? "DIFFERENT" : "same");
  FD_ZERO(&rd); FD_SET(s, &rd);
  tv.tv_sec = 2; tv.tv_usec = 0;
  k = select(s + 1, &rd, 0, 0, &tv);
  printf("NCTEST: select with nothing to read -> %d\n", k);
  send(s, "bye\n", 4, 0);
  FD_ZERO(&rd); FD_SET(s, &rd);
  tv.tv_sec = 5; tv.tv_usec = 0;
  k = select(s + 1, &rd, 0, 0, &tv);
  n = k > 0 ? recv(s, in, sizeof in, 0) : 0;
  printf("NCTEST: select after bye -> %d, recv %d bytes\n", k, n);
  closesocket(s);
  nicofclt_deinit();
  printf("NCTEST: done\n");
  return 0;
}
