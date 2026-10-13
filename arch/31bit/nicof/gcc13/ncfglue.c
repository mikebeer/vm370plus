/*
** NCFGLUE.C -- VM/370plus 9.5: the C side of NICOF's low-level interface
** for GCC 13 (-m31, cmsrt).  See intrapi.h and ncfintr.s.
**
** Released to the public domain, like NICOF.
*/

#include <string.h>
#include "intrapi.h"
#include "../../m5f/cp1047.h"

extern int ncf_low(int op, unsigned a, unsigned b);
extern void ncf_cr0(int on);
extern void ncf_exth(void);
extern void ncf_tmrx(void);
extern int cms202(void *plist);

#define NSLOT 32
#define HSZ 176
extern struct { _full in, out, lost, pad; char slot[NSLOT][HSZ]; } ncf_q;
extern void *ncf_hdrp;
extern _full *ncf_ecbp;
extern _full *ncf_tmrecb;

void ncf_a2e(char *dst, const char *src, int len)
{
  int i;
  for (i = 0; i < len; i++) dst[i] = rt_a2e[(unsigned char)src[i]];
}

void ncf_e2a(char *dst, const char *src, int len)
{
  int i;
  for (i = 0; i < len; i++) dst[i] = rt_e2a[(unsigned char)src[i]];
}

static char cmdbuf[240];          /* below 16 MB, EBCDIC */

void ncf_cpcmd(const char *cmd, int len)
{
  if (len > (int)sizeof(cmdbuf)) len = sizeof(cmdbuf);
  ncf_a2e(cmdbuf, cmd, len);
  ncf_low(2, (unsigned)cmdbuf, len);
}

void intrapi(void)
{
  CPexecuteCmd("SET TIMER REAL");
}

static struct { unsigned char name[8]; void *addr; } trapl;

static void hndext(void *addr)    /* HNDEXT SET / CLR */
{
  ncf_a2e((char *)trapl.name, "TRAP    ", 8);
  trapl.addr = addr;
  cms202(&trapl);
}

void ncf_enable_ext(void *hdr, int hdrlen, _full *ecb)
{
  (void)hdrlen;
  ncf_q.in = ncf_q.out = ncf_q.lost = 0;
  ncf_hdrp = hdr;
  ncf_ecbp = ecb;
  hndext((void *)ncf_exth);
  ncf_cr0(1);
}

void ncf_disable_ext(void)
{
  ncf_cr0(0);
  hndext(0);
  ncf_ecbp = 0;
}

int ncf_vmcf_next(void *buf)
{
  _full out = ncf_q.out;
  if (out == ncf_q.in) return 0;
  memcpy(buf, ncf_q.slot[out & (NSLOT - 1)], HSZ);
  ncf_q.out = out + 1;
  return 1;
}

void ncf_post(_full *ecb) { *ecb = 0x40000000; }

void ncf_wait(_full *ecb)
{
  ncf_low(3, 1, (unsigned)ecb);
}

void ncf_waitlist(_full **ecblist)
{
  ncf_low(3, 1, (unsigned)(-(int)(_full)ecblist));
}

static _full tmr_intv;
static _full tmr_zero;

void ncf_stimer(_full interval, _full *ecb)
{
  ncf_tmrecb = ecb;
  tmr_intv = interval;
  /* REAL (X'03') + BINTVL (X'10'), the exit address in bytes 1-3 */
  ncf_low(4, 0x13000000 | ((unsigned)ncf_tmrx & 0x00FFFFFF),
          (unsigned)&tmr_intv);
}

void ncf_streset(void)
{
  ncf_low(4, 0x13000000, (unsigned)&tmr_zero);
  ncf_tmrecb = 0;
}

int ncf_diag68(VMCPARM_PTR param)
{
  return ncf_low(1, (unsigned)param, 0);
}
