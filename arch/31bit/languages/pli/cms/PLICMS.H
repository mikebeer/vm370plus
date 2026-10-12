/* PLICMS H -- prelude to plirt.h (mikebeer/pli) for programs compiled
   on CMS (VM/370plus 9.20).  PL/I TITLEs and DD names are Unix style:
   "dir/name.ext" -> NAME EXT A (a bare name: NAME FILE A).
   GCC 13 (CC EXEC, __CMS__): cmsrt's fopen takes "name.ext"; a part
   longer than 8 becomes its first 4 and last 4 letters and digits.
   GCC380 with GCCLIB31: fopen takes "FN FT FM". */
#ifndef PLICMS_H
#define PLICMS_H
#ifdef __CMS__
#include <stdio.h>
#include <string.h>
#include <ctype.h>
/* one part of a CMS file id: letters and digits, and when there are more
   than 8 the first 4 and the last 4 ("plitest_out" -> PLITTOUT), so that
   names which differ only at the end stay apart */
static void pl_cmspart(const char *s, int n, char *out)
{
    char t[64];
    int k = 0, i;
    for (i = 0; i < n && k < 63; i++)
        if (isalnum((unsigned char)s[i]))
            t[k++] = (char)toupper((unsigned char)s[i]);
    t[k] = 0;
    if (k > 8) { memmove(t + 4, t + k - 4, 5); }
    strcpy(out, t);
}
static FILE *pl_cmsopen(const char *nm, const char *md)
{
    char fn[9], ft[9], id[24];
    const char *p = strrchr(nm, '/'), *dot;
    if (p) nm = p + 1;
    dot = strchr(nm, '.');
    pl_cmspart(nm, dot ? (int)(dot - nm) : (int)strlen(nm), fn);
    if (dot) pl_cmspart(dot + 1, (int)strlen(dot + 1), ft);
    else strcpy(ft, "FILE");
    if (!fn[0]) strcpy(fn, "PLIFILE");
    if (!ft[0]) strcpy(ft, "FILE");
    snprintf(id, sizeof id, "%s.%s", fn, ft);
    return fopen(id, md);
}
#define fopen(n, m) pl_cmsopen((n), (m))
#else
#include <stdio.h>
#include <string.h>
#include <ctype.h>
static FILE *pl_cmsopen(const char *nm, const char *md)
{
    char fn[9], ft[9], id[40];
    const char *p;
    int k, w;
    if (strchr(nm, ' ')) return fopen(nm, md);
    p = strrchr(nm, '/');
    if (p) nm = p + 1;
    for (k = 0; *nm && *nm != '.' && k < 8; nm++)
        fn[k++] = (char)toupper((unsigned char)*nm);
    fn[k] = 0;
    while (*nm && *nm != '.') nm++;
    if (*nm == '.') nm++;
    for (k = 0; *nm && *nm != '.' && k < 8; nm++)
        ft[k++] = (char)toupper((unsigned char)*nm);
    ft[k] = 0;
    if (!ft[0]) strcpy(ft, "FILE");
    w = md[0] != 'r' || strchr(md, '+') != 0;
    sprintf(id, w ? "%s %s A V 32760" : "%s %s *", fn, ft);
    return fopen(id, md);
}
#define fopen(n, m) pl_cmsopen((n), (m))
/* GCCLIB31's stdio.h has getc(s) as fgetc((c)) */
#undef getc
#define getc(s) fgetc(s)
#endif /* GCC380 */
#endif
