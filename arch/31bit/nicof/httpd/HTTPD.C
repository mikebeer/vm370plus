/*
** HTTPD C -- a small web server for CMS (VM/370plus 9.5), on the
** NICOF socket API (TCPIPPXY; the port is a port of the PC).
**
**   HTTPD [-p port] [-d fm] [-n count] [-k key] [-v]
**
**   GET /             INDEX HTML on fm if there is one, else a list of
**                     the files on fm (default A)
**   GET /fn.ft        the CMS file fn ft fm: text types are sent as
**                     ASCII lines, binary types (GIF PNG JPG ICO PDF ZIP
**                     BIN) as the record bytes
**   GET /.stop/key    ends the server when started with -k key
** -n count ends it after count requests.  HX also stops it.
** Build (CE GCC, IPL 290; NICOF's client rebuilt with CE GCC, see
** README): GCC HTTPD, GLOBAL TXTLIB GCCLIB,
**          LOAD HTTPD NICOFCLT NCFSOCKT INTRAPI (CLEAR, GENMOD HTTPD
** Run under IPL CMS (I-285).
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include "nicofclt.h"
#include "socket.h"


#define CHUNK 2000

static int verbose = 0;
static int lsock = -1, csock = -1;
static char fm[3] = "A";
static char req[4096], ebc[4096], buf[8192], out[8192];
static int flush(void);

/* the empty line that ends the request head, in ASCII (an EBCDIC '\n'
   is X'15', not the X'25' that LF becomes) */
static int endofhead(const char *p, int n)
{
  int i;
  for (i = 0; i + 1 < n; i++) {
    if (p[i] == 0x0a && p[i+1] == 0x0a) return 1;
    if (i + 3 < n && p[i] == 0x0d && p[i+1] == 0x0a && p[i+2] == 0x0d
        && p[i+3] == 0x0a) return 1;
  }
  return 0;
}

static void stop(int rc)
{
  if (csock >= 0) { flush(); closesocket(csock); }
  if (lsock >= 0) closesocket(lsock);
  nicofclt_deinit();
  exit(rc);
}

/* the output buffer: sends() collects translated text here and flush()
   sends it in CHUNK-sized blocks (one NICOF call per block, not per line) */
static int outn = 0;

static int flush(void)
{
  int r = 0;
  if (outn > 0) {
    if (verbose) printf("HTTPD: send %d bytes\n", outn);
    r = send(csock, out, outn, 0);
    if (verbose) printf("HTTPD: send -> %d\n", r);
    outn = 0;
  }
  return r < 0 ? -1 : 0;
}

/* send an EBCDIC C string as ASCII (buffered) */
static int sends(const char *s)
{
  for (; *s; s++) {           /* NICOF's table makes NL (X'15') a blank */
    out[outn++] = *s == '\n' ? 0x0a : e2a[(unsigned char)*s];
    if (outn >= CHUNK && flush() < 0) return -1;
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
  if (verbose) printf("HTTPD: fopen(%s)\n", name);
  f = fopen(name, bin ? "rb" : "r");
  if (verbose) printf("HTTPD: fopen -> %s\n", f ? "ok" : "failed");
  if (!f) return -1;
  header(200, "OK", type);
  if (bin) {
    if (flush() < 0) { fclose(f); return 0; }
    while ((n = fread(buf, 1, CHUNK, f)) > 0)
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

/* the next blank-delimited word, at most n characters */
static int word(char **pp, char *w, int n)
{
  char *p = *pp;
  int k = 0;
  while (*p == ' ') p++;
  while (*p && *p != ' ' && *p != '\n' && k < n) w[k++] = *p++;
  while (*p && *p != ' ' && *p != '\n') p++;
  w[k] = 0;
  *pp = p;
  return k > 0;
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
      if (word(&p, fn, 8) && word(&p, ft, 8) && word(&p, fmd, 2)) {
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
    if (argv[i][0] != '-') continue;
    if (o != 'V' && i + 1 >= argc) continue;
    if (o == 'P') port = atoi(argv[++i]);
    else if (o == 'D') { strncpy(fm, argv[++i], 2); fm[2] = 0; }
    else if (o == 'N') count = atoi(argv[++i]);
    else if (o == 'K') key = argv[++i];
    else if (o == 'V') verbose = 1;
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
      k = sizeof(req) - 1 - n;
      if (k > CHUNK) k = CHUNK;   /* NICOF moves at most 2 KB per call */
      k = recv(csock, req + n, k, 0);
      if (k <= 0) {
        if (n == 0) printf("HTTPD: recv() = %d, errno %d (%s)\n", k, errno,
                           nicofsocket_errmsg(errno));
        break;
      }
      n += k;
      if (endofhead(req, n)) break;
    }
    nicofclt_ascii2ebcdic(req, n, ebc);
    ebc[n] = 0;
    meth[0] = path[0] = 0;
    {                         /* "METHOD /path HTTP/1.x" (no sscanf:  */
      char *p = ebc, *q;      /* GCCLIB's %7s did not fill meth)      */
      for (q = meth; *p && *p != ' ' && q < meth + 7; ) *q++ = *p++;
      *q = 0;
      while (*p == ' ') p++;
      for (q = path; *p && *p != ' ' && *p != '\r' && q < path + 199; )
        *q++ = *p++;
      *q = 0;
    }
    printf("HTTPD: %s %s\n", meth, path);
    if (!meth[0] || path[0] != '/') {
      printf("HTTPD: %d bytes, %02X %02X %02X %02X / %02X %02X %02X %02X\n",
             n, req[0] & 255, req[1] & 255, req[2] & 255, req[3] & 255,
             ebc[0] & 255, ebc[1] & 255, ebc[2] & 255, ebc[3] & 255);
      flush(); closesocket(csock); csock = -1;
      continue;
    }
    if (key && !strncmp(path, "/.stop/", 7) && !strcmp(path + 7, key)) {
      header(200, "OK", "text/plain"); sends("HTTPD stopped\n");
      flush(); closesocket(csock); csock = -1;
      printf("HTTPD: stopped\n");
      stop(0);
    }
    if (strcmp(meth, "GET")) {
      header(501, "Not Implemented", "text/plain");
      sends("GET only\n");
    } else
      serve(path);
    flush(); closesocket(csock); csock = -1;
    if (count && ++served >= count) break;
  }
  stop(0);
  return 0;
}
