/*
** INTRAPI.H for GCC 13 (-m31, cmsrt) -- VM/370plus 9.5
**
** The low-level interface NICOF's client (NICOFCLT, NCFSOCKT) needs, for
** programs built with GCC 13 and the cmsrt runtime (arch/31bit/m5f) instead
** of CE's GCC and GCCLIB.  Same names and types as Hans-Walter Latz's
** INTRAPI.H, with these differences:
**
**  - the program is ASCII (Latin-1) inside; user ids and CP commands are
**    translated to EBCDIC here (SET_USER_FOR_CP, diagx08);
**  - there is no C external-interrupt handler.  The assembler handler
**    (ncfintr.s) runs in whatever mode DMSITE gives it (BC or EC, AMODE 24),
**    copies each VMCF interrupt header into a queue and posts one ECB;
**    the main line takes the queue apart (ncf_vmcf_next) in C;
**  - everything CP or CMS reads -- the VMCF parameter list and buffers,
**    the interrupt header, ECBs, ECB lists -- must be below 16 MB: static
**    data is (the image is loaded low), the heap and the C stack may not be.
**
** Released to the public domain, like NICOF.
*/

#ifndef __INTRAPI_INCLUDED
#define __INTRAPI_INCLUDED

#include <string.h>

typedef unsigned char      _byte;
typedef unsigned short     _half;
typedef unsigned int       _full;
typedef unsigned long long _dblw;
typedef void*              _addr;

/* Latin-1 <-> EBCDIC 1047 (ncfglue.c) */
extern void ncf_a2e(char *dst, const char *src, int len);
extern void ncf_e2a(char *dst, const char *src, int len);

/* CP SET TIMER REAL (for the timeout waits) */
extern void intrapi(void);

/* DIAGNOSE X'08': a CP command (ASCII, translated) */
extern void ncf_cpcmd(const char *cmd, int len);
#define diagx08(cpcmd, cpcmdlen) ncf_cpcmd(cpcmd, cpcmdlen)
#define CPexecuteCmd(cmd) ncf_cpcmd((cmd), strlen(cmd))

/* external interrupts: VMCF headers are queued by the assembler handler.
   enable: hdr is the VMCF header area given to AUTHORIZE (below 16 MB),
   hdrlen its size; ecb is posted on every VMCF interrupt. */
extern void ncf_enable_ext(void *hdr, int hdrlen, _full *ecb);
extern void ncf_disable_ext(void);
/* the next queued header (copied to buf), 0 if none */
extern int ncf_vmcf_next(void *buf);

/* ECBs (all below 16 MB: static) */
extern void ncf_post(_full *ecb);
extern void ncf_wait(_full *ecb);
extern void ncf_waitlist(_full **ecblist);
#define post_ecb(ecb) ncf_post(ecb)
#define wait_ecb(ecb) ncf_wait(ecb)
#define wait_anyecb(ecblist) ncf_waitlist(ecblist)
#define ECBLIST_ELEM(ecb) (_full*)(((_full)&ecb) & 0x7FFFFFFF)
#define ECBLIST_END(ecb)  (_full*)(((_full)&ecb) | 0x80000000)

/* the timer: post ecb after interval 1/100 s (STIMER REAL) */
extern void ncf_stimer(_full interval, _full *ecb);
extern void ncf_streset(void);
#define set_timer(interval,ecb) ncf_stimer(interval, ecb)
#define reset_timer() ncf_streset()

/*
** ***** VMCF interfacing
*/

typedef union {
  _dblw dblword;
  struct { _full w1; _full w2; } words;
  char  chars[8];
} DBLWORD;

/* the user id, EBCDIC, blank padded */
#define SET_USER_FOR_CP(dest,from) \
{ \
  int len = strlen(from); if (len > 8) { len = 8; } \
  memset(dest, ' ', 8); \
  memcpy(dest, from, len); \
  ncf_a2e(dest, dest, 8); \
}

typedef struct __vmcmhdr {
  _byte   v1;
  _byte   v2;
  _half   vmcmfunc;
  _full   vmcmmid;
  DBLWORD vmcmuser;
  _addr   vmcmvada;
  _full   vmcmlena;
  _addr   vmcmvadb;
  _full   vmcmlenb;
  DBLWORD vmcmuse;
  _byte   messageData[0];
} VMCMHDR, *VMCMHDR_PTR;

typedef struct __vmcparm {
  _byte   v1;
  _byte   v2;
  _half   vmcpfunc;
  _full   vmcpmid;
  DBLWORD vmcpuser;
  _addr   vmcpvada;
  _full   vmcplena;
  _addr   vmcpvadb;
  _full   vmcplenb;
  DBLWORD vmcpuse;
} VMCPARM, *VMCPARM_PTR;

/* flags for VMCMHDR.v1 */
#define VMCMRESP ((_byte)0x80)   /* final interrupt (transmission complete) */
#define VMCMRJCT ((_byte)0x40)   /* rejected by sink VM */
#define VMCMPRTY ((_byte)0x20)   /* priority message */

/* flags for VMCPARM.v1 */
#define VMCPAUTS ((_byte)0x80)   /* authorize specific (-> vmcpuser) */
#define VMCPPRTY ((_byte)0x40)   /* priority request / authorize priority */
#define VMCPSMSG ((_byte)0x20)   /* authorize SMSG messages */

/* constants for VMCPARM.vmcpfunc and VMCMHDR.vmcmfunc */
#define VMCPAUTH ((_half)0x0000)   /* authorize    */
#define VMCPUAUT ((_half)0x0001)   /* un-authorize */
#define VMCPSEND ((_half)0x0002)   /* send         */
#define VMCPSENR ((_half)0x0003)   /* send/receive */
#define VMCPSENX ((_half)0x0004)   /* sendx        */
#define VMCPRECV ((_half)0x0005)   /* receive      */
#define VMCPCANC ((_half)0x0006)   /* cancel       */
#define VMCPREPL ((_half)0x0007)   /* reply        */
#define VMCPQUIE ((_half)0x0008)   /* quiesce      */
#define VMCPRESM ((_half)0x0009)   /* resume       */
#define VMCPIDEN ((_half)0x000A)   /* identify     */
#define VMCPRJCT ((_half)0x000B)   /* reject       */

/* DIAGNOSE X'68' (the parameter list below 16 MB) */
extern int ncf_diag68(VMCPARM_PTR param);
#define vmcf_request(param) ncf_diag68(param)

#endif
