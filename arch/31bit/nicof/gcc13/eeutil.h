/*
** EEUTIL.H for GCC 13 (cmsrt) -- VM/370plus 9.5
** The part of Hans-Walter Latz's EE utilities that CMSFTPD uses, written
** anew for cmsrt (eeutil.c); EE's own sources are not part of NICOF.
** Disks and files are listed with CMS commands (CMSFTPD -usecmscmds,
** which this build always uses).
*/
#ifndef __EEUTIL31_INCLUDED
#define __EEUTIL31_INCLUDED
typedef void (*LineCallback)(char *line, void *cbdata);
extern char c_upper(char c);
extern char c_lower(char c);
extern void s_upper(char *src, char *dst);
extern void s_lower(char *src, char *dst);
extern int sncmp(char *s1, char *s2);
extern int f_exists(char *fn, char *ft, char *fm);
extern char *compileFidPattern(char *fn, char *ft, char *fm);
extern void getDiskList(LineCallback cb, void *cbdata);
extern void getFileList(LineCallback cb, void *cbdata, char *fn, char *ft, char *fm);
extern void diagx00(char *buf, int len);
#endif
