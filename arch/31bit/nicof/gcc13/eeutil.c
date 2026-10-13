/*
** EEUTIL.C -- VM/370plus 9.5: the EE utilities CMSFTPD needs, for GCC 13
** (cmsrt), plus GCCLIB's CMScommand, CMSstackQuery and CMSconsoleRead.
** Released to the public domain, like NICOF.
*/
#include <string.h>
#include <ctype.h>
#include "cmssys.h"
#include "intrapi.h"
#include "eeutil.h"

extern int cms202(void *plist);
extern int ncf_low(int op, unsigned a, unsigned b);

char c_upper(char c) { return toupper((unsigned char)c); }
char c_lower(char c) { return tolower((unsigned char)c); }
void s_upper(char *src, char *dst) { while ((*dst++ = toupper((unsigned char)*src))) src++; }
void s_lower(char *src, char *dst) { while ((*dst++ = tolower((unsigned char)*src))) src++; }

int sncmp(char *s1, char *s2)          /* case-insensitive strcmp */
{
  int a, b;
  do {
    a = toupper((unsigned char)*s1++); b = toupper((unsigned char)*s2++);
    if (a != b) return a < b ? -1 : 1;
  } while (a);
  return 0;
}

int f_exists(char *fn, char *ft, char *fm)
{
  char fid[19];
  int i;
  memset(fid, ' ', 18); fid[18] = 0;
  for (i = 0; i < 8 && fn[i]; i++) fid[i] = toupper((unsigned char)fn[i]);
  for (i = 0; i < 8 && ft[i]; i++) fid[8 + i] = toupper((unsigned char)ft[i]);
  fid[16] = fm && fm[0] ? toupper((unsigned char)fm[0]) : 'A';
  fid[17] = fm && fm[0] && fm[1] ? fm[1] : '*';
  return CMSfileState(fid, 0) == 0;
}

char *compileFidPattern(char *fn, char *ft, char *fm)
{
  (void)fm;
  if (!fn || !*fn || strlen(fn) > 8) return "invalid filename pattern";
  if (!ft || !*ft || strlen(ft) > 8) return "invalid filetype pattern";
  return 0;
}

/* never called: CMSFTPD lists with CMS commands in this build */
void getDiskList(LineCallback cb, void *cbdata) { (void)cb; (void)cbdata; }
void getFileList(LineCallback cb, void *cbdata, char *fn, char *ft, char *fm)
{ (void)cb; (void)cbdata; (void)fn; (void)ft; (void)fm; }

static char d0buf[64] __attribute__((aligned(8))); /* below 16 MB, DW */

void diagx00(char *buf, int len)       /* DIAGNOSE X'00', in ASCII */
{
  if (len > (int)sizeof d0buf) len = sizeof d0buf;
  memset(d0buf, 0, sizeof d0buf);
  ncf_low(5, (unsigned)d0buf, len);
  ncf_e2a(buf, d0buf, len);
}

static unsigned char pl[34][8];        /* tokenised plist, below 16 MB */

int CMScommand(char *command, int flag)
{
  char tok[9];
  int n = 0, k;
  char *p = command;
  (void)flag;
  memset(pl, 0xFF, sizeof pl);
  while (*p && n < 33) {
    while (*p == ' ') p++;
    if (!*p) break;
    k = 0;
    if (*p == '(' || *p == ')') tok[k++] = *p++;
    else
      while (*p && *p != ' ' && *p != '(' && *p != ')') {
        if (k < 8) tok[k++] = toupper((unsigned char)*p);
        p++;
      }
    memset(pl[n], 0x40, 8);
    ncf_a2e((char *)pl[n], tok, k);
    n++;
  }
  return cms202(pl);
}

static unsigned char senpl[16] = { 0xE2,0xC5,0xD5,0xE3,0xD9,0xC9,0xC5,0xE2, /* SENTRIES */
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };

int CMSstackQuery(void)
{
  int n = cms202(senpl);
  return n > 0 ? n : 0;
}

static struct {                        /* WAITRD plist (RDTERM EDIT=NO) */
  unsigned char name[8];
  unsigned int buf;
  unsigned char code, code2;
  unsigned short len;
} rdpl;
static char rdbuf[136];

int CMSconsoleRead(char *line)
{
  int n;
  ncf_a2e((char *)rdpl.name, "WAITRD  ", 8);
  rdpl.buf = 0x01000000 | (unsigned)rdbuf;
  rdpl.code = 0xE3;                    /* 'T': as typed */
  rdpl.code2 = 0xF0;
  rdpl.len = 0;
  cms202(&rdpl);
  n = rdpl.len;
  if (n < 0 || n > 130) n = 0;
  ncf_e2a(line, rdbuf, n);
  line[n] = 0;
  return n;
}
