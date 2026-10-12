/* xl.c -- VM/370plus XL loader: runs a large M5f program from high storage.
 *
 *   XL fn [args...]      loads fn XIMAGE (mkximage.py) and runs it
 *
 * CMS LOAD puts a module below 16 MB, so a program of tens of megabytes
 * (GCC's cc1) cannot be a module.  This small module (a normal M5f program
 * below 16 MB) reads the image into its heap above 16 MB, adds the load
 * address to every address constant, clears the BSS and calls the image's
 * cms_xl_main() in AMODE 31.  It hands over what must stay below 16 MB:
 * its own cms202 (SVC 202 runs in AMODE 24) and a block for plists, the
 * FSCB and record buffers (cmsrt.c, struct lowdata).  The program's exit
 * comes back here through xl_exit (longjmp). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

typedef unsigned int u32;

/* as in cmsrt.c */
#define RECMAX 32760
struct lowdata {
    unsigned char tline[130];
    unsigned char typl[16];
    unsigned char rdpl[16];
    unsigned char rdbuf[132];
    struct { char cmd[8], fn[8]; u32 bytes, addr; } hsp;
    struct { unsigned char b[48]; } fscb;
    unsigned char dirpl[8 * 7];
    unsigned char rec[RECMAX + 8];
} __attribute__((aligned(8)));
struct xlsvc {
    int (*cms202)(void *plist);
    void (*exit)(int rc);
    struct lowdata *low;
    int argc;
    char **argv;
};

int cms202(void *plist);
static struct lowdata low;
static jmp_buf back;
static int xl_rc;

/* the loader's own heap holds the image: keep it to what that needs and
   leave the rest of HIGHSTOR to the program */
u32 cms_heap_mb = 112;
unsigned int cms_stack_mb = 1;

static void xl_exit(int rc)
{
    xl_rc = rc;
    longjmp(back, 1);
}

static u32 be32(const unsigned char *p)
{
    return (u32)p[0] << 24 | (u32)p[1] << 16 | (u32)p[2] << 8 | p[3];
}

int main(int argc, char **argv)
{
    char name[64];
    FILE *f;
    long n;
    unsigned char *file, *img;
    u32 entry, isize, bsize, nrel, i;
    struct xlsvc svc;
    int (*go)(struct xlsvc *);

    if (argc < 2) {
        fprintf(stderr, "XL: give the program: XL fn [args]  (fn XIMAGE)\n");
        return 24;
    }
    snprintf(name, sizeof name, "%s.ximage(b", argv[1]);
    f = fopen(name, "rb");
    if (!f) { fprintf(stderr, "XL: %s XIMAGE not found\n", argv[1]); return 28; }
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    file = malloc(n);
    if (!file || fread(file, 1, n, f) != (size_t)n) {
        fprintf(stderr, "XL: cannot read %s XIMAGE (%ld bytes)\n", argv[1], n);
        return 104;
    }
    fclose(f);
    if (n < 24 || memcmp(file, "XIMG", 4) || be32(file + 4) != 1) {
        fprintf(stderr, "XL: %s XIMAGE is not an XIMAGE\n", argv[1]);
        return 32;
    }
    entry = be32(file + 8); isize = be32(file + 12);
    bsize = be32(file + 16); nrel = be32(file + 20);
    if (24 + isize + 4 * nrel > (u32)n) {
        fprintf(stderr, "XL: %s XIMAGE is truncated\n", argv[1]);
        return 32;
    }
    img = malloc(isize + bsize + 8);
    if (!img) {
        fprintf(stderr, "XL: no storage for %u bytes\n", isize + bsize);
        return 104;
    }
    memcpy(img, file + 24, isize);
    memset(img + isize, 0, bsize);
    for (i = 0; i < nrel; i++) {
        u32 off = be32(file + 24 + isize + 4 * i);
        unsigned char *p = img + off;
        u32 v = be32(p) + (u32)img;
        p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v;
    }
    free(file);
    fflush(stdout);
    fflush(stderr);
    svc.cms202 = cms202;
    svc.exit = xl_exit;
    svc.low = &low;
    svc.argc = argc - 1;
    svc.argv = argv + 1;
    go = (int (*)(struct xlsvc *))(img + entry);
    if (setjmp(back) == 0) {
        go(&svc);
        xl_rc = 0;
    }
    free(img);
    return xl_rc;
}
