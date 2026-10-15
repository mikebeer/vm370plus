/* VM/370plus 9.39: EXECCOMM for Regina on CMS -- the CMS shared variable
 * interface (EXECIO STEM/VAR, GLOBALV GET/PUT, any program that calls
 * EXECCOMM) reaches the variables of the running Regina program.
 *
 * Registered at start-up (cmsrt's cms_program_init hook) as the SUBCOM environment
 * EXECCOMM through cmsrt's cms_subcom(); dropped at exit, so BREXX's (or
 * an outer Regina's) is in force again.  Called with the extended plist,
 * whose fourth word is the first SHVBLOCK (EBCDIC names and values):
 *   +0 SHVNEXT  +8 SHVCODE  +9 SHVRET  +12 SHVBUFL  +16 SHVNAMA
 *   +20 SHVNAML  +24 SHVVALA  +28 SHVVALL
 * Codes S/s set, F/f fetch, D/d drop (upper case direct, lower symbolic).
 * SHVRET: X'01' new variable, X'04' truncated, X'08' bad name, X'80' bad
 * function; R15 is the OR of them, less NEWV and LVAR (EXECIO takes any
 * other non-zero as an error). */
#include "rexx.h"
#include <string.h>

int cms_subcom(const char *name, int (*fn)(unsigned char *, unsigned char *));
void cms_e2a(char *p, int n);
void cms_a2e(char *p, int n);

static unsigned int be32(const unsigned char *p)
{
    return (unsigned)p[0] << 24 | (unsigned)p[1] << 16 | (unsigned)p[2] << 8 | p[3];
}
static void put32(unsigned char *p, unsigned v)
{
    p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v;
}

static int regina_execcomm(unsigned char *plist, unsigned char *eplist)
{
    tsd_t *TSD = __regina_get_tsd();
    unsigned char *shv;
    int rc = 0;
    (void)plist;
    eplist = (unsigned char *)((unsigned)eplist & 0x7FFFFFFF);
    if (!eplist || !TSD || !TSD->systeminfo || !TSD->systeminfo->tree.root)
        return -1;
    shv = (unsigned char *)(be32(eplist + 12) & 0x7FFFFFFF);
    for (; shv; shv = (unsigned char *)(be32(shv) & 0x7FFFFFFF)) {
        char code, namebuf[256];
        unsigned nama = be32(shv + 16), naml = be32(shv + 20);
        unsigned vala = be32(shv + 24), vall = be32(shv + 28), bufl = be32(shv + 12);
        streng *nm;
        code = (char)shv[8];
        cms_e2a(&code, 1);
        shv[9] = 0;
        if (naml == 0 || naml > 250) { shv[9] = 0x08; rc |= 0x08; continue; }
        memcpy(namebuf, (char *)(nama & 0x7FFFFFFF), naml);
        cms_e2a(namebuf, naml);
        for (unsigned i = 0; i < naml; i++)
            if (namebuf[i] >= 'a' && namebuf[i] <= 'z') namebuf[i] -= 32;
        nm = Str_ncreTSD(namebuf, naml);
        switch (code) {
        case 'S': case 's': {
            streng *v = Str_makeTSD(vall);
            memcpy(v->value, (char *)(vala & 0x7FFFFFFF), vall);
            v->len = vall;
            cms_e2a(v->value, vall);
            if (code == 's') setvalue(TSD, nm, v, -1); else setdirvalue(TSD, nm, v);
            if (!var_was_found(TSD)) shv[9] |= 0x01;
            break;
        }
        case 'F': case 'f': {
            const streng *v = code == 'f' ? getvalue(TSD, nm, -1) : getdirvalue(TSD, nm);
            unsigned n = v ? (unsigned)v->len : 0;
            if (!var_was_found(TSD)) shv[9] |= 0x01;
            if (n > bufl) { n = bufl; shv[9] |= 0x04; }
            if (n) {
                memcpy((char *)(vala & 0x7FFFFFFF), v->value, n);
                cms_a2e((char *)(vala & 0x7FFFFFFF), n);
            }
            put32(shv + 28, n);
            break;
        }
        case 'D': case 'd':
            if (code == 'd') drop_var(TSD, nm); else drop_dirvar(TSD, nm);
            if (!var_was_found(TSD)) shv[9] |= 0x01;
            break;
        default:
            shv[9] |= 0x80;
        }
        Free_stringTSD(nm);
        rc |= shv[9] & ~0x03;                 /* NEWV, LVAR: not errors */
    }
    return rc;
}

void cms_program_init(void)
{
    cms_subcom("EXECCOMM", regina_execcomm);
}
