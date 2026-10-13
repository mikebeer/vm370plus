/*
** HTTPD C -- a small web server for CMS (VM/370plus 9.5), on the
** NICOF socket API (TCPIPPXY; the port is a port of the PC).
**
**   HTTPD [-p port] [-d fm] [-n count] [-k key]
**
**   GET /             INDEX HTML on fm if there is one, else a list of
**                     the files on fm (default A)
**   GET /fn.ft        the CMS file fn ft fm: text types are sent as
**                     ASCII lines, binary types (GIF PNG JPG ICO PDF ZIP
**                     BIN) as the record bytes
**   GET /.stop/key    ends the server when started with -k key
** -n count ends it after count requests.  HX also stops it.
** Build (CE GCC):  GCC HTTPD, then  GLOBAL TXTLIB NICOFLIB GCCLIB,
**                  LOAD HTTPD (CLEAR, GENMOD HTTPD
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include "nicofclt.h"
#include "socket.h"

static int lsock = -1, csock = -1;
static char fm[3] = "A";
static char req[4096], ebc[4096], buf[8192], out[8192];

static void stop(int rc)
{
  if (csock >= 0) closesocket(csock);
  if (lsock >= 0) closesocket(lsock);
  nicofclt_deinit();
  exit(rc);
}

/* send an EBCDIC C string as ASCII */
static int sends(const char *s)
{
  int n = strlen(s);
  while (n > 0) {
    int k = n > (int)sizeof(out) ? (int)sizeof(out) : n;
    nicofclt_ebcdic2ascii((char*)s, k, out);
    if (send(csock, out, k, 0) < 0) return -1;
    s += k; n -= k;
  }
  return 0;
}

static void header(int code, const char *msg, const char *type)
{
  char h[256];
  sprintf(h, "HTTP/1.0 %d %s\r\nServer: VM/370plus CMS HTTPD\r\n"
             "Content-Type: %s\r\nConnection: close\r\n\r\n",
          code, msg, type);
  sends(h);
}

static void notfound(const char *what)
{
  char h[200];
  header(404, "Not Found", "text/html");
  sprintf(h, "<html><body><h1>404</h1><p>%.100s not found on "
             "VM/370plus.</p></body></html>\n", what);
  sends(h);
}

static const char *bintypes[] = { "GIF", "PNG", "JPG", "JPEG", "ICO", "PDF",
  "ZIP", "BIN", 0 };

static const char *ctype(const char *ft, int *bin)
{
  int i;
  *bin = 0;
  for (i = 0; bintypes[i]; i++) if (!strcmp(ft, bintypes[i])) *bin = 1;
  if (!strcmp(ft, "HTML") || !strcmp(ft, "HTM")) return "text/html";
  if (!strcmp(ft, "CSS")) return "text/css";
  if (!strcmp(ft, "JS")) return "text/javascript";
  if (!strcmp(ft, "GIF")) return "image/gif";
  if (!strcmp(ft, "PNG")) return "image/png";
  if (!strcmp(ft, "JPG") || !strcmp(ft, "JPEG")) return "image/jpeg";
  if (!strcmp(ft, "ICO")) return "image/x-icon";
  if (!strcmp(ft, "PDF")) return "application/pdf";
  if (*bin) return "application/octet-stream";
  return "text/plain";
}

static int sendfile(const char *fn, const char *ft, const char *fmode)
{
  char name[40];
  int bin, n;
  const char *type = ctype(ft, &bin);
  FILE *f;
  sprintf(name, "%s %s %s", fn, ft, fmode);
  f = fopen(name, bin ? "rb" : "r");
  if (!f) return -1;
  header(200, "OK", type);
  if (bin) {
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
      if (send(csock, buf, n, 0) < 0) break;
  } else {
    while (fgets(buf, sizeof(buf) - 2, f)) {
      n = strlen(buf);
      while (n > 0 && (buf[n-1] == '\n' || buf[n-1] == ' ')) n--;
      buf[n++] = '\n'; buf[n] = 0;
      if (sends(buf) < 0) break;
    }
  }
  fclose(f);
  return 0;
}

/* the files on the disk, from LISTFILE * * fm (EXEC -> CMS EXEC A1 */
static void listing(void)
{
  char cmd[40], line[200], fn[9], ft[9], fmd[3];
  FILE *f;
  sprintf(cmd, "LISTFILE * * %s (EXEC", fm);
  system(cmd);
  header(200, "OK", "text/html");
  sprintf(line, "<html><head><title>VM/370plus: %s disk</title></head>"
                "<body><h1>Files on the %s disk</h1><pre>\n", fm, fm);
  sends(line);
  f = fopen("CMS EXEC A1", "r");
  if (f) {
    while (fgets(buf, sizeof(buf), f)) {
      char *p = buf;
      while (*p == ' ' || *p == '&') {   /* "&1 &2 fn ft fm" */
        if (*p == '&') { while (*p && *p != ' ') p++; } else p++;
      }
      if (sscanf(p, "%8s %8s %2s", fn, ft, fmd) == 3) {
        sprintf(line, "<a href=\"/%s.%s\">%-8s %-8s</a> %s\n",
                fn, ft, fn, ft, fmd);
        sends(line);
      }
    }
    fclose(f);
  }
  sends("</pre><p>Served by CMS on VM/370plus (NICOF sockets).</p>"
        "</body></html>\n");
}

static void serve(char *path)
{
  char fn[9], ft[9], fmode[3], *dot, *dot2;
  int i;
  for (i = 0; path[i]; i++) path[i] = toupper(path[i]);
  if (!strcmp(path, "/")) {
    if (sendfile("INDEX", "HTML", fm) < 0) listing();
    return;
  }
  path++;
  strcpy(fmode, fm);
  dot = strchr(path, '.');
  if (!dot || dot - path > 8) { notfound(path - 1); return; }
  *dot = 0;
  dot2 = strchr(dot + 1, '.');
  if (dot2) { *dot2 = 0; strncpy(fmode, dot2 + 1, 2); fmode[2] = 0; }
  strncpy(fn, path, 8); fn[8] = 0;
  strncpy(ft, dot + 1, 8); ft[8] = 0;
  if (sendfile(fn, ft, fmode) < 0) { *dot = '.'; notfound(path); }
}

int main(int argc, char *argv[])
{
  struct sockaddr_in a;
  int port = 8080, count = 0, served = 0, i;
  char *key = 0;
  for (i = 1; i < argc; i++) {
    char o = toupper(argv[i][1]);
    if (argv[i][0] != '-' || i + 1 >= argc) continue;
    if (o == 'P') port = atoi(argv[++i]);
    else if (o == 'D') { strncpy(fm, argv[++i], 2); fm[2] = 0; }
    else if (o == 'N') count = atoi(argv[++i]);
    else if (o == 'K') key = argv[++i];
  }
  for (i = 0; fm[i]; i++) fm[i] = toupper(fm[i]);
  nicofclt_init();
  lsock = socket(AF_INET, SOCK_STREAM, 0);
  if (lsock < 0) {
    printf("HTTPD: socket() failed: %s\n", nicofsocket_errmsg(errno));
    stop(36);
  }
  a.sin_family = AF_INET;
  a.sin_port = htons(port);
  a.sin_addr.s_addr = inet_addr("0.0.0.0");
  if (bind(lsock, (struct sockaddr*)&a, sizeof(a)) < 0) {
    printf("HTTPD: bind() to port %d failed: %s\n", port,
           nicofsocket_errmsg(errno));
    stop(40);
  }
  if (listen(lsock, 5) < 0) {
    printf("HTTPD: listen() failed: %s\n", nicofsocket_errmsg(errno));
    stop(42);
  }
  printf("HTTPD: serving the %s disk on port %d\n", fm, port);
  for (;;) {
    struct sockaddr ca;
    int clen = sizeof(ca), n = 0, k;
    char meth[8], path[200];
    csock = accept(lsock, &ca, &clen);
    if (csock < 0) {
      printf("HTTPD: accept() failed: %s\n", nicofsocket_errmsg(errno));
      stop(44);
    }
    /* the request up to the empty line (or what fits) */
    while (n < (int)sizeof(req) - 1) {
      k = recv(csock, req + n, sizeof(req) - 1 - n, 0);
      if (k <= 0) break;
      n += k;
      req[n] = 0;
      nicofclt_ascii2ebcdic(req, n, ebc);
      ebc[n] = 0;
      if (strstr(ebc, "\r\n\r\n") || strstr(ebc, "\n\n")) break;
    }
    ebc[n] = 0;
    meth[0] = path[0] = 0;
    sscanf(ebc, "%7s %199s", meth, path);
    printf("HTTPD: %s %s\n", meth, path);
    if (key && !strncmp(path, "/.stop/", 7) && !strcmp(path + 7, key)) {
      header(200, "OK", "text/plain"); sends("HTTPD stopped\n");
      closesocket(csock); csock = -1;
      printf("HTTPD: stopped\n");
      stop(0);
    }
    if (strcmp(meth, "GET")) {
      header(501, "Not Implemented", "text/plain");
      sends("GET only\n");
    } else
      serve(path);
    closesocket(csock); csock = -1;
    if (count && ++served >= count) break;
  }
  stop(0);
  return 0;
}
