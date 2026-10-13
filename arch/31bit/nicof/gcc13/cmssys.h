/*
** CMSSYS.H for GCC 13 (cmsrt) -- VM/370plus 9.5
**
** The part of CE GCCLIB's CMS interface that NICOF's tools use (NHFS, HACC,
** CMSFTPD), on top of cmsrt.  File ids and commands are ASCII here and
** translated to EBCDIC; record data is passed as is (the caller translates
** text records, see nhfscomn.c).  CMSFILE structures and buffers must be
** below 16 MB (static), because CMS addresses them with 24 bits.
**
** Released to the public domain, like NICOF.
*/
#ifndef __CMSSYS31_INCLUDED
#define __CMSSYS31_INCLUDED

#include <stdlib.h>

/* CMSFILE maps the CMS FSCB */
typedef struct {
    char command[8];
    char filename[8];
    char filetype[8];
    char filemode[2];
    short recordNum;
    char *buffer;
    int bufferSize;
    char format[2];
    short numRecords;
    int bytesRead;
} CMSFILE;

/* CMSFILEINFO maps the CMS FST (names and format translated to ASCII) */
typedef struct {
    char filename[8];
    char filetype[8];
    short filedate;
    short filetime;
    short writePtr;
    short readptr;
    char filemode[2];
    short numRecords;
    short chainLink;
    char format;
    char flag;
    int lrecl;
    short numBlocks;
    short fileYear;
} CMSFILEINFO;

extern int CMSfileOpen(char *fileid, char *buffer, int bufferSize, char format,
                       int numRecords, int recordNum, CMSFILE *file);
extern int CMSfileRead(CMSFILE *file, int recordNum, int *bytesRead);
extern int CMSfileWrite(CMSFILE *file, int recordNum, int recordLen);
extern int CMSfileClose(CMSFILE *file);
extern int CMSfileState(char *fileid, CMSFILEINFO **fileInfo);
extern int CMSfileErase(char *fileid);
extern int CMSfileRename(char *oldFileid, char *newFileid);
extern int CMScommand(char *command, int flag);
#define CMS_COMMAND 0
#define CMS_FUNCTION 1
#define CMS_CONSOLE 2

#define __exit(rc) exit(rc)

#endif
