/*
** CMSFILE.C -- VM/370plus 9.5: CE GCCLIB's CMS file functions on cmsrt
** (GCC 13), for NICOF's tools.  See cmssys.h.
**
** File ids are GCCLIB's 18-character form: fn in 0-7, ft in 8-15, fm in
** 16-17, blank padded (nhfscomn.c's buildFid).
**
** Released to the public domain, like NICOF.
*/
#include <string.h>
#include "cmssys.h"
#include "intrapi.h"

extern int cms202(void *plist);

static void put8a(char *d, const char *s, int n)  /* ASCII -> EBCDIC, padded */
{
  int i;
  char t[8];
  memset(t, ' ', 8);
  for (i = 0; i < n && s[i]; i++)
    t[i] = s[i] >= 'a' && s[i] <= 'z' ? s[i] - 32 : s[i];
  ncf_a2e(d, t, 8);
}

static void fid2fscb(const char *fid, CMSFILE *f)
{
  char fm[8];
  put8a(f->filename, fid, 8);
  put8a(f->filetype, fid + 8, 8);
  memset(fm, ' ', 8);
  fm[0] = fid[16] >= 'a' && fid[16] <= 'z' ? fid[16] - 32 : fid[16];
  fm[1] = fid[17];
  if (fm[0] == ' ' || fm[0] == 0) fm[0] = 'A';
  if (fm[1] == ' ' || fm[1] == 0) fm[1] = fm[0] == '*' ? ' ' : '1';
  ncf_a2e(f->filemode, fm, 2);
}

static void cmd8(CMSFILE *f, const char *cmd)
{
  put8a(f->command, cmd, 8);
}

static CMSFILE st;               /* STATE plist (an FSCB), below 16 MB */
static CMSFILEINFO stinfo;

int CMSfileState(char *fileid, CMSFILEINFO **fileInfo)
{
  int rc;
  memset(&st, 0, sizeof st);
  cmd8(&st, "STATE");
  fid2fscb(fileid, &st);
  rc = cms202(&st);
  if (rc == 0 && st.buffer) {
    memcpy(&stinfo, st.buffer, sizeof stinfo);
    ncf_e2a(stinfo.filename, stinfo.filename, 8);
    ncf_e2a(stinfo.filetype, stinfo.filetype, 8);
    ncf_e2a(stinfo.filemode, stinfo.filemode, 2);
    ncf_e2a(&stinfo.format, &stinfo.format, 1);
    if (fileInfo) *fileInfo = &stinfo;
  }
  return rc;
}

int CMSfileOpen(char *fileid, char *buffer, int bufferSize, char format,
                int numRecords, int recordNum, CMSFILE *file)
{
  char fv[2];
  int rc;
  memset(file, 0, sizeof *file);
  fid2fscb(fileid, file);
  file->buffer = buffer;
  file->bufferSize = bufferSize;
  fv[0] = format == 'F' || format == 'f' ? 'F' : 'V';
  fv[1] = ' ';
  ncf_a2e(file->format, fv, 2);
  file->numRecords = numRecords;
  file->recordNum = recordNum;
  rc = CMSfileState(fileid, 0);
  return rc == 0 ? 0 : rc;     /* 28: a new file (for writing) */
}

int CMSfileRead(CMSFILE *file, int recordNum, int *bytesRead)
{
  int rc;
  cmd8(file, "RDBUF");
  if (recordNum != -1) file->recordNum = recordNum;
  file->bytesRead = 0;
  rc = cms202(file);
  if (bytesRead) *bytesRead = file->bytesRead;
  return rc;
}

int CMSfileWrite(CMSFILE *file, int recordNum, int recordLen)
{
  cmd8(file, "WRBUF");
  if (recordNum != -1) file->recordNum = recordNum;
  file->bufferSize = recordLen;
  return cms202(file);
}

int CMSfileClose(CMSFILE *file)
{
  cmd8(file, "FINIS");
  return cms202(file);
}

static struct { char tok[9][8]; } pl;  /* tokenised plist, below 16 MB */

static int fidcmd(const char *cmd, const char *a, const char *b)
{
  CMSFILE t;
  int n = 0;
  memset(&pl, 0xFF, sizeof pl);
  put8a(pl.tok[n++], cmd, 8);
  fid2fscb(a, &t);
  memcpy(pl.tok[n++], t.filename, 8);
  memcpy(pl.tok[n++], t.filetype, 8);
  memset(pl.tok[n], 0x40, 8); memcpy(pl.tok[n++], t.filemode, 2);
  if (b) {
    fid2fscb(b, &t);
    memcpy(pl.tok[n++], t.filename, 8);
    memcpy(pl.tok[n++], t.filetype, 8);
    memset(pl.tok[n], 0x40, 8); memcpy(pl.tok[n++], t.filemode, 2);
  }
  return cms202(&pl);
}

int CMSfileErase(char *fileid) { return fidcmd("ERASE", fileid, 0); }

int CMSfileRename(char *oldFileid, char *newFileid)
{
  return fidcmd("RENAME", oldFileid, newFileid);
}
