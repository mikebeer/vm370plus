/* cmsrt.c -- VM/370plus M5f: the CMS side of newlib for GCC -m31 programs.
 *
 * The program runs in AMODE 31 with its code, data, stack and plists in the
 * image CMS LOADed below 16 MB, and its heap above 16 MB from HIGHSTOR.
 * Every CMS service goes through cms202() (entry31.s): AMODE 24, SVC 202,
 * back to 31.  So nothing handed to CMS may live in the heap: plists,
 * record buffers and the terminal line are static.
 *
 * The C side is ASCII (Latin-1); CMS is EBCDIC (code page 037).  Text is
 * translated at the boundary (IBM-1047, as CE's Hercules CODEPAGE 819/1047): terminal lines, file records, command tokens.
 *
 * Files are whole-file: open for reading reads every record into the heap
 * (text: EBCDIC -> ASCII, F-format trailing blanks dropped, one '\n' per
 * record; binary: records concatenated), so read/lseek are memory
 * operations.  Open for writing collects the bytes and writes them at close
 * (text: one V record per line, an empty line is one blank; binary: V
 * records of up to RECMAX bytes).  Binary = filetype RXBIN or MODULE, or a
 * name ending in "(B)" -- newlib does not pass fopen's 'b' down.
 * Names: "fn.ft[.fm]" or "fn ft [fm]", case-insensitive, fm default A1.
 */
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/time.h>
#include <time.h>
#include "cp1047.h"
#define POSIXRC(call) do { int r_ = (call); if (r_ < 0) *__errno() = errno; return r_; } while (0)

/* M8: cREXX's mainframe entry points call mainframe_set_text_conversion(0)
 * and do IBM-1047 themselves: raw record bytes, ASCII LF as the delimiter. */
static int rt_conv = 1;
void mainframe_set_text_conversion(int enabled) { rt_conv = enabled != 0; }
#define A2E(c) (rt_conv ? rt_a2e[(unsigned char)(c)] : (unsigned char)(c))
#define E2A(c) (rt_conv ? rt_e2a[(unsigned char)(c)] : (char)(c))

#undef errno
extern int errno;

int cms202(void *plist);
void cms_exit(int rc) __attribute__((noreturn));
int cms_onstack(void *top, int (*fn)(int, char **), int argc, char **argv);
int main(int, char **);
/* main runs on a stack of cms_stack_mb MB from the heap (above 16 MB):
   the 64 KB start-up stack is far too small for cREXX's recursion. */
unsigned int cms_stack_mb __attribute__((weak)) = 8;
/* C++ static constructors (.init_array, kept by image.ld/imagexl.ld) */
extern void (*__init_array_start[])(void) __attribute__((weak));
extern void (*__init_array_end[])(void) __attribute__((weak));
static void cms_init_array(void)
{
    void (**f)(void);
    if (!__init_array_start) return;
    for (f = __init_array_start; f < __init_array_end; f++) (*f)();
}
static int cms_run_main(int argc, char **argv)
{
    cms_init_array();
    char *s = malloc(cms_stack_mb << 20);
    if (!s) return main(argc, argv);
    return cms_onstack(s + (cms_stack_mb << 20), main, argc, argv);
}
int main(int argc, char **argv);

typedef unsigned int u32;

/* ------------------------------------------------------------ low data */
/* Everything handed to CMS (plists, FSCB, record and terminal buffers) is in
 * one block.  Normally it is static, in the image below 16 MB.  A large
 * program (XL, built with -DCMS_XL) runs from high storage: the XL loader,
 * itself a small module below 16 MB, passes a low block and its own cms202
 * (whose AMODE 24 instructions must be below 16 MB too). */
#define RECMAX 32760
struct fscb {
    unsigned char cmd[8], fn[8], ft[8], fm[2];
    unsigned short recno;
    u32 buf, bsize;
    unsigned char recfm[2];
    unsigned short norec;
    u32 nread;
};
struct lowdata {
    unsigned char tline[130];
    unsigned char typl[16];
    unsigned char rdpl[16];
    unsigned char rdbuf[132];
    struct { char cmd[8], fn[8]; u32 bytes, addr; } hsp;
    struct fscb fscb;
    unsigned char dirpl[8 * 7];
    unsigned char rec[RECMAX + 8];
} __attribute__((aligned(8)));
#ifdef CMS_XL
struct xlsvc {                      /* from the XL loader (xl.c) */
    int (*cms202)(void *plist);
    void (*exit)(int rc);
    struct lowdata *low;
    int argc;
    char **argv;
};
static struct xlsvc *xl;
static struct lowdata *L;
int cms202(void *plist) { return xl->cms202(plist); }
#else
static struct lowdata lowstatic;
static struct lowdata *L = &lowstatic;
#endif
#define tline (L->tline)
#define typl (L->typl)
#define rdpl (L->rdpl)
#define rdbuf (L->rdbuf)
#define hsp (L->hsp)
#define fscb (L->fscb)
#define rec (L->rec)

/* ---------------------------------------------------------------- terminal */
static int tlen;

static void put8(unsigned char *d, const char *s)
{
    int i = 0;
    for (; s[i] && i < 8; i++) d[i] = rt_a2e[(unsigned char)s[i]];
    for (; i < 8; i++) d[i] = 0x40;
}

static void tflush(void)
{
    u32 a = (u32)tline;
    if (tlen == 0) { tline[0] = 0x40; tlen = 1; }
    put8(typl, "TYPLIN");
    typl[8] = 1; typl[9] = a >> 16; typl[10] = a >> 8; typl[11] = a;
    typl[12] = 0xC2; typl[13] = 0; typl[14] = tlen >> 8; typl[15] = tlen;
    cms202(typl);
    tlen = 0;
}

static void tput(const char *p, int n)
{
    for (int i = 0; i < n; i++) {
        unsigned char c = p[i];
        if (c == '\n') { tflush(); continue; }
        if (c == '\r') continue;
        if (tlen == (int)sizeof tline) tflush();
        tline[tlen++] = c == '\t' && rt_conv ? 0x40 : A2E(c);
    }
}

/* RDTERM: one terminal line, translated, plus '\n' */
static char tin[136];
static int tin_len, tin_pos;

static int tread(char *p, int n)
{
    if (tin_pos >= tin_len) {
        u32 a = (u32)rdbuf;
        int len;
        if (tlen) tflush();
        memset(rdbuf, 0x40, sizeof rdbuf);
        put8(rdpl, "WAITRD");
        rdpl[8] = 1; rdpl[9] = a >> 16; rdpl[10] = a >> 8; rdpl[11] = a;
        rdpl[12] = 0xE3; /* 'T': EDIT=NO, as typed */
        rdpl[13] = 0xF0; rdpl[14] = 0; rdpl[15] = 0;
        if (cms202(rdpl)) return 0;
        len = rdpl[14] << 8 | rdpl[15];             /* length read */
        if (len > 130) len = 130;
        for (int i = 0; i < len; i++) tin[i] = E2A(rdbuf[i]);
        /* a line of just "/*" is end of file (as in CMS's OS simulation):
           a program reading stdin to its end can be fed from the stack */
        if (len == 2 && tin[0] == '/' && tin[1] == '*') return 0;
        tin[len] = '\n';
        tin_len = len + 1; tin_pos = 0;
    }
    int k = tin_len - tin_pos;
    if (k > n) k = n;
    memcpy(p, tin + tin_pos, k);
    tin_pos += k;
    return k;
}

/* ---------------------------------------------------------------- heap */
static char *heap_base, *heap_end, *heap_brk;
static char low_heap[64 << 10] __attribute__((aligned(8)));

static int highstor(const char *fn, u32 bytes, u32 addr)
{
    put8((unsigned char *)hsp.cmd, "HIGHSTOR");
    put8((unsigned char *)hsp.fn, fn);
    hsp.bytes = bytes; hsp.addr = addr;
    return cms202(&hsp);
}

u32 cms_heap_mb __attribute__((weak)) = 0;        /* 0: the largest that HIGHSTOR gives, up to 232 MB */

static void heap_init(void)
{
    static const u32 tries[] = { 232, 224, 192, 160, 128, 96, 64, 48, 32, 16, 0 };
    if (cms_heap_mb) {
        if (!highstor("OBTAIN", cms_heap_mb << 20, 0)) {
            heap_base = (char *)hsp.addr; heap_end = heap_base + (cms_heap_mb << 20);
        }
    } else {
        for (int i = 0; tries[i]; i++)
            if (!highstor("OBTAIN", tries[i] << 20, 0)) {
                heap_base = (char *)hsp.addr; heap_end = heap_base + (tries[i] << 20);
                break;
            }
    }
    if (!heap_base) {
        static const char m[] = "CMSRT: no HIGHSTOR storage above 16 MB; 64 KB heap only\n";
        tput(m, sizeof m - 1); tflush();
        heap_base = low_heap; heap_end = low_heap + sizeof low_heap;
    }
    heap_brk = heap_base;
}

static void heap_fini(void)
{
    if (heap_base && heap_base != low_heap)
        highstor("RELEASE", heap_end - heap_base, (u32)heap_base);
    heap_base = 0;
}

void *_sbrk(int incr)
{
    char *old = heap_brk;
    if (!heap_base) heap_init();
    old = heap_brk;
    if (incr < 0 || heap_brk + incr > heap_end) {
        static int told;
        if (!told++) {          /* say so once: a program may not check */
            static const char m[] = "CMSRT: out of storage (heap above 16 MB is full)\n";
            tput(m, sizeof m - 1);
            tflush();
        }
        errno = ENOMEM; return (void *)-1;
    }
    heap_brk += incr;
    return old;
}

void cms_debug_heap(const char *w)
{
    char b[160];
    int n = snprintf(b, sizeof b, "CMSRT %s: heap %p-%p brk %p sp %p\n", w,
                     (void *)heap_base, (void *)heap_end, (void *)heap_brk, (void *)&n);
    tput(b, n);
}

/* ---------------------------------------------------------------- files */
#define NFILES 16

struct cfile {
    int used, writing, binary, fm_given, fixed;
    unsigned int ino;
    unsigned char fn[8], ft[8], fm[2];
    char *data;
    u32 len, cap, pos;
};
static struct cfile files[NFILES];

static int parse_name(const char *name, struct cfile *f)
{
    char part[3][9];
    int np = 0, k = 0;
    const char *p = name;
    char dirmode[3] = {0, 0, 0};
    memset(part, 0, sizeof part);
    f->binary = 0; f->fixed = 0;
    while (*p == ' ') p++;
    /* "/tmp/..." (and "/var/tmp/..."): scratch files are CMS files on the
       accessed disks (written to A), named by the last component */
    if (!strncmp(p, "/tmp/", 5) || !strncmp(p, "/var/tmp/", 9)) p = strrchr(p, '/') + 1;
    /* "dir/fn.ft": cREXX's openfile() builds these.  "." is no mode (search
       the accessed disks); otherwise the directory is the filemode letter. */
    const char *slash = strrchr(p, '/');
    if (slash) {
        if (!(slash - p == 1 && p[0] == '.') && slash > p) {
            dirmode[0] = p[0];
            if (slash - p > 1 && p[1] >= '0' && p[1] <= '9') dirmode[1] = p[1];
        }
        p = slash + 1;
    }
    for (; *p; p++) {
        if (*p == '(') { if ((p[1] == 'b' || p[1] == 'B')) f->binary = 1; break; }
        if (*p == '.' || *p == ' ') {
            if (k) { np++; k = 0; }
            if (np == 3) break;
            continue;
        }
        if (np >= 3) return -1;
        if (k >= 8) continue;   /* longer names are cut to 8, as CMS does */
        char c = *p;
        if (c >= 'a' && c <= 'z') c -= 32;
        part[np][k++] = c;
    }
    if (k) np++;
    /* a bare name is a ddname, as CMS FILEDEF defaults it: "OUTDD" is
       FILE OUTDD (A when written) */
    if (np == 1) { memcpy(part[1], part[0], 9); memcpy(part[0], "FILE\0\0\0\0", 9); np = 2; }
    if (np < 2) return -1;
    put8(f->fn, part[0]); put8(f->ft, part[1]);
    if (np >= 3) {
        f->fm[0] = rt_a2e[(unsigned char)part[2][0]];
        f->fm[1] = part[2][1] ? rt_a2e[(unsigned char)part[2][1]] : 0xF1;
        f->fm_given = 1;
    } else if (dirmode[0]) {
        char c = dirmode[0];
        if (c >= 'a' && c <= 'z') c -= 32;
        f->fm[0] = rt_a2e[(unsigned char)c];
        f->fm[1] = dirmode[1] ? rt_a2e[(unsigned char)dirmode[1]] : 0xF1;
        f->fm_given = 1;
    } else { f->fm[0] = 0xC1; f->fm[1] = 0xF1; f->fm_given = 0; }
    /* binary by file type: object files, archives and images (the C/C++
       toolchain's as/ld/ar read and write them through fopen) */
    static const char *const bin[] = { "RXBIN", "MODULE", "O", "A", "ELF", "OBJ",
        "XIMAGE", "LIB", "BIN", "OUT", "GCH", "TAR", "TMP", "DB", "SQLITE", "DB3", "DBF", "DBT", "XDX",
        "ZIP", "GZ", "TGZ", "Z", "BZ2", "XZ", "TRS", "TERSE", "JAR", 0 };
    for (int b = 0; bin[b]; b++) if (!strcmp(part[1], bin[b])) f->binary = 1;
    /* SQLite's journal of fn.db is fn.db-journal: filetype DB-JOURN */
    if (!strncmp(part[1], "DB-", 3)) f->binary = 1;
    /* gzip's fn.ft-gz (GZ_SUFFIX "-gz" on CMS): filetype ending in -GZ */
    { size_t n = strlen(part[1]); if (n >= 3 && !strcmp(part[1] + n - 3, "-GZ")) f->binary = 1; }
    /* TEXT decks (elf_to_cms output): EBCDIC card images, F 80 */
    if (!strcmp(part[1], "TEXT")) { f->binary = 1; f->fixed = 80; }
    return 0;
}

static void set_fscb(const char *cmd, struct cfile *f)
{
    memset(&fscb, 0, sizeof fscb);
    put8(fscb.cmd, cmd);
    memcpy(fscb.fn, f->fn, 8); memcpy(fscb.ft, f->ft, 8); memcpy(fscb.fm, f->fm, 2);
}

static int grow(struct cfile *f, u32 more)
{
    if (f->len + more <= f->cap) return 0;
    u32 n = f->cap ? f->cap * 2 : 4096;
    while (n < f->len + more) n *= 2;
    char *d = realloc(f->data, n);
    if (!d) return -1;
    f->data = d; f->cap = n;
    return 0;
}

static int read_whole(struct cfile *f)
{
    for (;;) {
        int rc;
        set_fscb("RDBUF", f);
        fscb.buf = (u32)rec; fscb.bsize = RECMAX;
        fscb.recfm[0] = 0xE5; fscb.recfm[1] = 0x40;   /* 'V ' -- CMS sets it */
        fscb.norec = 1;
        rc = cms202(&fscb);
        if (rc == 12) break;                           /* end of file */
        if (rc) { return rc == 1 || rc == 2 || rc == 28 ? -ENOENT : -EIO; }
        u32 n = fscb.nread;
        if (grow(f, n + 1)) return -ENOMEM;
        if (f->binary) {
            memcpy(f->data + f->len, rec, n); f->len += n;
        } else {
            while (n > 0 && rec[n - 1] == 0x40) n--;  /* F records are padded */
            for (u32 i = 0; i < n; i++) f->data[f->len++] = E2A(rec[i]);
            f->data[f->len++] = '\n';
        }
    }
    /* close it: CMS keeps a read position per open file */
    set_fscb("FINIS", f);
    cms202(&fscb);
    return 0;
}

static int write_rec(struct cfile *f, const unsigned char *p, u32 n)
{
    set_fscb("WRBUF", f);
    fscb.buf = (u32)p; fscb.bsize = n;
    fscb.recfm[0] = f->fixed ? 0xC6 : 0xE5; fscb.recfm[1] = 0x40;  /* F / V */
    fscb.norec = 1;
    return cms202(&fscb);
}

static int write_whole(struct cfile *f)
{
    int rc = 0;
    u32 i = 0;
    set_fscb("ERASE", f);
    cms202(&fscb);
    if (f->binary) {
        while (i < f->len && !rc) {
            u32 n = f->len - i > RECMAX ? RECMAX : f->len - i;
            if (f->fixed) n = f->len - i > f->fixed ? f->fixed : f->len - i;
            memcpy(rec, f->data + i, n);
            if (f->fixed && n < f->fixed) { memset(rec + n, 0x40, f->fixed - n); n = f->fixed; }
            rc = write_rec(f, rec, n);
            i += n;
        }
    } else {
        while (i < f->len && !rc) {
            u32 n = 0;
            while (i < f->len && f->data[i] != '\n') {
                if (n < RECMAX) rec[n++] = A2E(f->data[i]);
                i++;
            }
            if (i < f->len) i++;                      /* the '\n' */
            if (n == 0) rec[n++] = 0x40;
            rc = write_rec(f, rec, n);
        }
    }
    set_fscb("FINIS", f);
    cms202(&fscb);
    return rc ? -EIO : 0;
}

/* st_ino: programs compare (st_dev, st_ino) to spot the same file twice
   (GCC's include directories, elf_to_cms's input/output check) */
static unsigned int ino_hash(const unsigned char *p, int n)
{
    unsigned int h = 2166136261u;
    for (int i = 0; i < n; i++) { h ^= p[i]; h *= 16777619u; }
    return h ? h : 1;
}

/* ---------------------------------------------------- the sysroot (VFS)
 * Absolute paths ("/usr/include/stdio.h") are looked up, read-only, in a
 * tar file on an accessed disk: SYSROOT TAR, or the file named by the
 * environment variable CMS_SYSROOT ("fn.ft").  The C/C++ toolchain finds
 * its headers and libraries there; CMS file ids stay flat. */
/* Two tars: SYSROOT TAR (C: /usr/include, /usr/lib), and CXXROOT TAR for
 * C++ (/usr/include/c++, /usr/lib/cxx), read only when a C++ path is
 * looked up, so a C compile does not carry the C++ headers in storage. */
struct vroot { const char *name, *env, *prefix; char *tar; u32 len; int loaded; };
static struct vroot vroots[2] = {
    { "CXXROOT TAR", "CMS_CXXROOT", "usr/include/c++", 0, 0, 0 },
    { "SYSROOT TAR", "CMS_SYSROOT", "", 0, 0, 0 },
};
static u32 octal(const char *p, int n)
{
    u32 v = 0;
    for (int i = 0; i < n && p[i]; i++) if (p[i] >= '0' && p[i] <= '7') v = v * 8 + (p[i] - '0');
    return v;
}
static void vfs_load(struct vroot *r)
{
    struct cfile g;
    const char *n = getenv(r->env);
    r->loaded = 1;
    memset(&g, 0, sizeof g);
    if (parse_name(n ? n : r->name, &g)) return;
    g.binary = 1;
    if (!g.fm_given) g.fm[0] = 0x5C;
    if (read_whole(&g)) { free(g.data); return; }
    r->tar = g.data; r->len = g.len;
}
#define TMPPATH(n) (!strncmp((n), "/tmp/", 5) || !strncmp((n), "/var/tmp/", 9))
static int vfs_pre(const char *path, const char *pre)
{
    size_t n = strlen(pre);
    return !strncmp(path, pre, n) && (path[n] == 0 || path[n] == '/');
}
/* the member's data and size; *isdir for a directory (an entry, or a
   prefix of one) */
static const char *vfs_find(const char *path, u32 *size, int *isdir)
{
    while (*path == '/') path++;
    struct vroot *r = &vroots[1];
    if (vfs_pre(path, vroots[0].prefix) || vfs_pre(path, "usr/lib/cxx")) r = &vroots[0];
    if (!r->loaded) vfs_load(r);
    if (!r->tar) return 0;
    size_t pl = strlen(path);
    while (pl && path[pl - 1] == '/') pl--;
    for (u32 off = 0; off + 512 <= r->len; ) {
        const char *h = r->tar + off;
        if (!h[0]) break;
        char nm[260];
        int k = 0;
        if (!memcmp(h + 257, "ustar", 5) && h[345]) {
            for (int i = 0; i < 155 && h[345 + i]; i++) nm[k++] = h[345 + i];
            nm[k++] = '/';
        }
        for (int i = 0; i < 100 && h[i]; i++) nm[k++] = h[i];
        nm[k] = 0;
        char *q = nm;
        while (*q == '.' && q[1] == '/') q += 2;
        while (*q == '/') q++;
        size_t ql = strlen(q);
        while (ql && q[ql - 1] == '/') ql--;
        u32 sz = octal(h + 124, 12);
        char type = h[156];
        if (ql == pl && !memcmp(q, path, pl)) {
            *isdir = type == '5';
            *size = sz;
            return h + 512;
        }
        if (ql > pl && !memcmp(q, path, pl) && q[pl] == '/') { *isdir = 1; *size = 0; return h; }
        off += 512 + ((sz + 511) & ~511u);
    }
    return 0;
}

int _open(const char *name, int flags, int mode)
{
    int fd;
    (void)mode;
    for (fd = 3; fd < NFILES && files[fd].used; fd++) ;
    if (fd == NFILES) { errno = EMFILE; return -1; }
    struct cfile *f = &files[fd];
    memset(f, 0, sizeof *f);
    /* "/x.y" (one component) is a CMS file: SQLite makes every name
       absolute against getcwd(), which is "." */
    if (name[0] == '/' && name[1] && strchr(name + 1, '/') && !TMPPATH(name)) {
        u32 sz; int dir;
        const char *d = vfs_find(name, &sz, &dir);
        if (!d || dir) { errno = d ? EISDIR : ENOENT; return -1; }
        if ((flags & O_ACCMODE) != O_RDONLY) { errno = EROFS; return -1; }
        f->data = malloc(sz + 1);
        if (!f->data) { errno = ENOMEM; return -1; }
        memcpy(f->data, d, sz);
        f->len = sz; f->cap = sz + 1; f->binary = 1; f->used = 1;
        f->ino = ino_hash((const unsigned char *)name, strlen(name));
        return fd;
    }
    if (parse_name(name, f)) { errno = EINVAL; return -1; }
    f->used = 1;
    if ((flags & O_ACCMODE) == O_RDONLY) {
        if (!f->fm_given) f->fm[0] = 0x5C;            /* '*': any accessed disk */
        int rc = read_whole(f);
        if (rc) { free(f->data); f->used = 0; errno = -rc; return -1; }
    } else {
        f->writing = 1;
        /* Without O_TRUNC the file keeps its contents ("r+", "a", and
           GnuCOBOL's OPEN I-O / EXTEND): read it first, position at 0 or,
           for O_APPEND, at the end. */
        if (!(flags & O_TRUNC)) {
            struct cfile g = *f;
            if (!g.fm_given) g.fm[0] = 0x5C;
            if (read_whole(&g) == 0) {
                f->data = g.data; f->len = g.len; f->cap = g.cap;
                f->pos = (flags & O_APPEND) ? f->len : 0;
            }
        }
    }
    return fd;
}

int _close(int fd)
{
    if (fd < 3) return 0;
    if (fd >= NFILES || !files[fd].used) { errno = EBADF; return -1; }
    struct cfile *f = &files[fd];
    int rc = f->writing ? write_whole(f) : 0;
    free(f->data);
    f->used = 0;
    if (rc) { errno = -rc; return -1; }
    return 0;
}

int _write(int fd, const char *p, int n)
{
    if (fd == 1 || fd == 2) { tput(p, n); return n; }
    if (fd < 3 || fd >= NFILES || !files[fd].used || !files[fd].writing) { errno = EBADF; return -1; }
    struct cfile *f = &files[fd];
    if (f->pos + n > f->len && grow(f, f->pos + n - f->len)) { errno = ENOSPC; return -1; }
    if (f->pos > f->len) memset(f->data + f->len, 0, f->pos - f->len);   /* the gap */
    memcpy(f->data + f->pos, p, n);
    f->pos += n;
    if (f->pos > f->len) f->len = f->pos;
    return n;
}

int _read(int fd, char *p, int n)
{
    if (fd == 0) return tread(p, n);
    if (fd < 3 || fd >= NFILES || !files[fd].used) { errno = EBADF; return -1; }
    struct cfile *f = &files[fd];
    if (f->pos >= f->len) return 0;
    u32 k = f->len - f->pos;
    if ((u32)n < k) k = n;
    memcpy(p, f->data + f->pos, k);
    f->pos += k;
    return k;
}

int _lseek(int fd, int off, int whence)
{
    if (fd < 3) return 0;
    if (fd >= NFILES || !files[fd].used) { errno = EBADF; return -1; }
    struct cfile *f = &files[fd];
    long np = whence == SEEK_SET ? off : whence == SEEK_CUR ? (long)f->pos + off : (long)f->len + off;
    if (np < 0) { errno = EINVAL; return -1; }
    /* past the end: the file grows only when written there (POSIX) --
       SQLite seeks to offset 24 of an empty database to read it */
    if ((u32)np > f->len && !f->writing) np = f->len;
    f->pos = np;
    return np;
}

/* the time a CMS file was last written, from its FST: STATE leaves the
   address of its copy of the 40-byte FST (STATEFST) in the plist at +28
   (DMSSTT).  Packed MMDD at +16, HHMM at +18, the year as EBCDIC digits at
   +38.  0 if the file is not on disk. */
static int bcd2(unsigned char b) { return (b >> 4) * 10 + (b & 15); }
static time_t cms_mtime(const struct cfile *f)
{
    static unsigned int plw[8];                      /* static: below 16 MB */
    unsigned char *pl = (unsigned char *)plw;
    static const unsigned char state[8] = { 0xE2,0xE3,0xC1,0xE3,0xC5,0x40,0x40,0x40 };
    memcpy(pl, state, 8); memcpy(pl + 8, f->fn, 8); memcpy(pl + 16, f->ft, 8);
    pl[24] = f->fm_given ? f->fm[0] : 0x5C;          /* '*': any disk */
    pl[25] = f->fm_given && f->fm[1] && f->fm[1] != 0x40 ? f->fm[1] : 0x40;
    pl[26] = pl[27] = 0x40;
    plw[7] = 0;
    if (cms202(pl) || !plw[7]) return 0;
    const unsigned char *e = (const unsigned char *)(plw[7] & 0x00FFFFFF);
    int mon = bcd2(e[16]), day = bcd2(e[17]), hh = bcd2(e[18]), mi = bcd2(e[19]);
    int yy = (e[38] & 15) * 10 + (e[39] & 15);
    if (mon < 1 || mon > 12 || day < 1 || day > 31) return 0;
    static const int cum[12] = { 0,31,59,90,120,151,181,212,243,273,304,334 };
    int y = (yy < 70 ? 2000 : 1900) + yy;
    long days = (long)(y - 1970) * 365 + ((y - 1969) / 4) + cum[mon - 1] + day - 1;
    if (mon > 2 && y % 4 == 0) days++;
    return (time_t)(days * 86400L + hh * 3600L + mi * 60L);
}

int _fstat(int fd, struct stat *st)
{
    memset(st, 0, sizeof *st);
    if (fd < 3) { st->st_mode = S_IFCHR; return 0; }
    if (fd >= NFILES || !files[fd].used) { errno = EBADF; return -1; }
    st->st_mode = S_IFREG | 0644; st->st_size = files[fd].len; st->st_blksize = 4096;
    st->st_nlink = 1;
    st->st_dev = 1;
    st->st_ino = files[fd].ino ? files[fd].ino : ino_hash(files[fd].fn, 16);
    if (!files[fd].writing && files[fd].fn[0]) st->st_mtime = st->st_atime = st->st_ctime = cms_mtime(&files[fd]);
    return 0;
}

int _stat(const char *name, struct stat *st)
{
    /* "." (and "./", "/") is the set of accessed disks: a directory, so
       programs that check a work directory (GnuCOBOL's TMPDIR) accept it */
    if (!strcmp(name, ".") || !strcmp(name, "./") || !strcmp(name, "/")) {
        memset(st, 0, sizeof *st); st->st_mode = S_IFDIR | 0777; return 0;
    }
    if (name[0] == '/' && strchr(name + 1, '/') && !TMPPATH(name)) {
        u32 sz; int dir;
        if (!vfs_find(name, &sz, &dir)) { errno = ENOENT; return -1; }
        memset(st, 0, sizeof *st);
        st->st_mode = dir ? S_IFDIR | 0555 : S_IFREG | 0444;
        st->st_size = sz; st->st_blksize = 4096;
        st->st_dev = 2; st->st_ino = ino_hash((const unsigned char *)name, strlen(name));
        return 0;
    }
    /* a file this program has open for writing is what it says it is,
       on disk or not yet (CMS writes it at close or fsync): SQLite stats
       its new database to check that it was not unlinked or moved */
    {
        struct cfile t;
        memset(&t, 0, sizeof t);
        if (!parse_name(name, &t))
            for (int k = 3; k < NFILES; k++)
                if (files[k].used && files[k].writing && !memcmp(files[k].fn, t.fn, 8) &&
                    !memcmp(files[k].ft, t.ft, 8) && (!t.fm_given || files[k].fm[0] == t.fm[0]))
                    return _fstat(k, st);
    }
    int fd = _open(name, O_RDONLY, 0);
    if (fd < 0) return -1;
    _fstat(fd, st);
    _close(fd);
    return 0;
}

int stat(const char *name, struct stat *st) { POSIXRC(_stat(name, st)); }
int fstat(int fd, struct stat *st) { POSIXRC(_fstat(fd, st)); }
int _isatty(int fd) { return fd < 3; }

int _unlink(const char *name)
{
    struct cfile f;
    memset(&f, 0, sizeof f);
    if (parse_name(name, &f)) { errno = EINVAL; return -1; }
    set_fscb("ERASE", &f);
    if (cms202(&fscb)) { errno = ENOENT; return -1; }
    return 0;
}

/* No processes on CMS: system() would LOAD a MODULE over the running one. */
int system(const char *cmd) { return cmd ? -1 : 0; }
int _execve(const char *n, char *const a[], char *const e[]) { (void)n; (void)a; (void)e; errno = ENOSYS; return -1; }
int _fork(void) { errno = ENOSYS; return -1; }
int _wait(int *st) { (void)st; errno = ECHILD; return -1; }
int write(int fd, const void *p, size_t n) { return _write(fd, p, n); }

/* POSIX names for programs that use them directly (binutils, GCC).  Weak:
   a program's own version wins (GnuCOBOL's cobcms.c opens record files
   in binary).  open() is binary unless the name says otherwise. */
#include <stdarg.h>
/* newlib code reads errno through __errno() (the reentrancy struct);
   the _xxx functions above set the plain global, as newlib's own _r
   wrappers expect.  Direct POSIX calls must copy it over. */
__attribute__((weak)) int open(const char *name, int flags, ...)
{
    va_list ap; int mode;
    va_start(ap, flags); mode = va_arg(ap, int); va_end(ap);
    POSIXRC(_open(name, flags, mode));
}
__attribute__((weak)) int close(int fd) { POSIXRC(_close(fd)); }
__attribute__((weak)) int read(int fd, void *p, size_t n) { POSIXRC(_read(fd, p, n)); }
__attribute__((weak)) off_t lseek(int fd, off_t off, int whence) { POSIXRC(_lseek(fd, off, whence)); }
__attribute__((weak)) int unlink(const char *name) { POSIXRC(_unlink(name)); }
__attribute__((weak)) int isatty(int fd) { return _isatty(fd); }
/* fsync writes the file to its disk now (CMS keeps it in storage until
   close): SQLite's journal depends on it */
__attribute__((weak)) int fsync(int fd)
{
    if (fd < 3) return 0;
    if (fd >= NFILES || !files[fd].used) { *__errno() = EBADF; return -1; }
    if (!files[fd].writing) return 0;
    int rc = write_whole(&files[fd]);
    if (rc) { *__errno() = -rc; return -1; }
    return 0;
}
__attribute__((weak)) int ftruncate(int fd, off_t n)
{
    if (fd < 3 || fd >= NFILES || !files[fd].used || !files[fd].writing || n < 0) { *__errno() = EBADF; return -1; }
    struct cfile *f = &files[fd];
    if ((u32)n > f->len) {
        if (grow(f, n - f->len)) { *__errno() = ENOSPC; return -1; }
        memset(f->data + f->len, 0, n - f->len);
    }
    f->len = n;
    if (f->pos > f->len) f->pos = f->len;
    return 0;
}
/* one user, no owners, no links, no processes */
__attribute__((weak)) int fchown(int fd, uid_t u, gid_t g) { (void)fd; (void)u; (void)g; return 0; }
__attribute__((weak)) uid_t getuid(void) { return 1; }
__attribute__((weak)) uid_t geteuid(void) { return 1; }
__attribute__((weak)) void *getpwuid(uid_t u) { (void)u; return 0; }
__attribute__((weak)) int getrusage(int who, void *ru) { (void)who; memset(ru, 0, 72); return 0; }
__attribute__((weak)) int nanosleep(const struct timespec *req, struct timespec *rem) { (void)req; (void)rem; return 0; }
__attribute__((weak)) FILE *popen(const char *c, const char *m) { (void)c; (void)m; *__errno() = ENOSYS; return 0; }
__attribute__((weak)) int pclose(FILE *f) { (void)f; *__errno() = ECHILD; return -1; }
__attribute__((weak)) int readlink(const char *p, char *b, size_t n) { (void)p; (void)b; (void)n; *__errno() = EINVAL; return -1; }
__attribute__((weak)) int symlink(const char *a, const char *b) { (void)a; (void)b; *__errno() = ENOSYS; return -1; }
__attribute__((weak)) int ioctl(int fd, unsigned long r, ...) { (void)fd; (void)r; *__errno() = ENOTTY; return -1; }
__attribute__((weak)) int getpid(void) { return 1; }
__attribute__((weak)) mode_t umask(mode_t m) { (void)m; return 022; }
__attribute__((weak)) int chmod(const char *n, mode_t m) { (void)n; (void)m; return 0; }
/* fcntl: one user, so every lock is granted and none is ever held by
   another (F_GETLK answers F_UNLCK, as SQLite's locking asks) */
__attribute__((weak)) int fcntl(int fd, int cmd, ...)
{
    (void)fd;
    if (cmd == F_GETLK) {
        va_list ap;
        va_start(ap, cmd);
        struct flock *l = va_arg(ap, struct flock *);
        va_end(ap);
        if (l) l->l_type = F_UNLCK;
    }
    return 0;
}
__attribute__((weak)) long sysconf(int name) { return name == 8 /* _SC_PAGESIZE */ ? 4096 : 16; }
__attribute__((weak)) int lstat(const char *name, struct stat *st) { POSIXRC(_stat(name, st)); }
__attribute__((weak)) int access(const char *name, int mode) { struct stat st; (void)mode; POSIXRC(_stat(name, &st)); }
__attribute__((weak)) char *getcwd(char *buf, size_t n)
{
    if (!buf || n < 2) { errno = ERANGE; return 0; }
    strcpy(buf, "."); return buf;
}
__attribute__((weak)) char *getwd(char *buf) { strcpy(buf, "."); return buf; }
/* mkstemp: a Unix template ("out.text.tmp.XXXXXX") is no CMS file id;
   the template is rewritten to TMPnnnnn TMP (it never grows) */
static unsigned int gettimeofday_usec(void);
int mkstemp(char *tpl)
{
    static unsigned int seq;
    size_t n = strlen(tpl);
    if (n < 8) { errno = EINVAL; return -1; }
    for (int tries = 0; tries < 100; tries++) {
        unsigned int k = (unsigned int)(gettimeofday_usec() + seq++) % 100000u;
        if (n >= 12) sprintf(tpl, "TMP%05u.TMP", k);
        else sprintf(tpl, "T%03u.TMP", k % 1000u);      /* zip's "ziXXXXXX" */
        struct stat st;
        if (_stat(tpl, &st) == 0) continue;
        int fd = _open(tpl, O_RDWR | O_CREAT | O_TRUNC, 0600);
        if (fd >= 0) { files[fd].binary = 1; return fd; }
    }
    errno = EEXIST;
    return -1;
}
/* the rest of newlib's mktemp.o, so that it is never linked beside ours
   (libiberty's make_temp_file calls mkstemps) */
int mkstemps(char *tpl, int suffixlen) { (void)suffixlen; return mkstemp(tpl); }
int mkostemp(char *tpl, int flags) { (void)flags; return mkstemp(tpl); }
int mkostemps(char *tpl, int suffixlen, int flags) { (void)suffixlen; (void)flags; return mkstemp(tpl); }
char *mktemp(char *tpl)
{
    int fd = mkstemp(tpl);
    if (fd < 0) { tpl[0] = 0; return tpl; }
    _close(fd); _unlink(tpl);
    return tpl;
}
char *mkdtemp(char *tpl) { (void)tpl; errno = ENOSYS; return 0; }   /* no directories */

/* CMS file ids are already canonical; sysroot paths are absolute */
__attribute__((weak)) char *realpath(const char *name, char *resolved)
{
    if (!name) { errno = EINVAL; return 0; }
    if (!resolved && !(resolved = malloc(strlen(name) + 1))) { errno = ENOMEM; return 0; }
    strcpy(resolved, name);
    return resolved;
}
__attribute__((weak)) int dup(int fd) { (void)fd; errno = EMFILE; return -1; }
__attribute__((weak)) int dup2(int a, int b) { (void)a; (void)b; errno = EBADF; return -1; }
__attribute__((weak)) int pipe(int *fds) { (void)fds; errno = EMFILE; return -1; }
__attribute__((weak)) int mkdir(const char *n, mode_t m) { (void)n; (void)m; errno = EACCES; return -1; }
__attribute__((weak)) int rmdir(const char *n) { (void)n; errno = EACCES; return -1; }
__attribute__((weak)) int chdir(const char *n) { (void)n; errno = ENOENT; return -1; }
__attribute__((weak)) int fchmod(int fd, mode_t m) { (void)fd; (void)m; return 0; }
__attribute__((weak)) int fork(void) { errno = ENOSYS; return -1; }
__attribute__((weak)) int execv(const char *p, char *const a[]) { (void)p; (void)a; errno = ENOSYS; return -1; }
__attribute__((weak)) int execvp(const char *p, char *const a[]) { (void)p; (void)a; errno = ENOSYS; return -1; }
__attribute__((weak)) int wait(int *s) { (void)s; errno = ECHILD; return -1; }
__attribute__((weak)) int kill(int pid, int sig) { (void)pid; (void)sig; errno = EPERM; return -1; }
__attribute__((weak)) unsigned sleep(unsigned s) { (void)s; return 0; }
__attribute__((weak)) unsigned alarm(unsigned s) { (void)s; return 0; }
__attribute__((weak)) int ftw(const char *d, void *f, int n) { (void)d; (void)f; (void)n; errno = ENOSYS; return -1; }
__attribute__((weak)) int nftw(const char *d, void *f, int n, int fl) { (void)d; (void)f; (void)n; (void)fl; errno = ENOSYS; return -1; }
struct utimbuf;
__attribute__((weak)) int utime(const char *n, const struct utimbuf *t) { (void)n; (void)t; return 0; }
__attribute__((weak)) int utimes(const char *n, const struct timeval *t) { (void)n; (void)t; return 0; }

/* rename: the file's contents under the new name, then the old one erased
   (CMS RENAME is a transient command and may not be called from here) */
int _rename(const char *from, const char *to)
{
    int a = _open(from, O_RDONLY, 0);
    if (a < 0) return -1;
    struct cfile *fa = &files[a];
    int b = _open(to, O_WRONLY | O_CREAT | O_TRUNC, 0);
    if (b < 0) { _close(a); return -1; }
    files[b].binary = fa->binary;
    if (_write(b, fa->data, fa->len) != (int)fa->len) { _close(a); _close(b); errno = EIO; return -1; }
    _close(a);
    if (_close(b)) return -1;
    return _unlink(from);
}
__attribute__((weak)) int rename(const char *from, const char *to) { return _rename(from, to); }

int _link(const char *a, const char *b) { (void)a; (void)b; errno = EMLINK; return -1; }
int _kill(int pid, int sig) { (void)pid; (void)sig; errno = EINVAL; return -1; }
int _getpid(void) { return 1; }
clock_t _times(struct tms *t) { memset(t, 0, sizeof *t); return 0; }

/* TOD clock: bit 51 = 1 microsecond, epoch 1900 */
static unsigned int gettimeofday_usec(void)
{
    unsigned long long tod;
    __asm__ volatile ("stck %0" : "=Q"(tod) : : "cc");
    return (unsigned int)(tod >> 12);
}
int gettimeofday(struct timeval *tv, void *tz)
{
    unsigned long long tod;
    (void)tz;
    __asm__ volatile ("stck %0" : "=Q"(tod) : : "cc");
    unsigned long long us = tod >> 12;
    us -= 2208988800ULL * 1000000ULL;
    tv->tv_sec = (long)(us / 1000000ULL);
    tv->tv_usec = (long)(us % 1000000ULL);
    return 0;
}

void _exit(int rc)
{
    for (int fd = 3; fd < NFILES; fd++) if (files[fd].used) _close(fd);
    if (tlen) tflush();
    heap_fini();
    cms_exit(rc);
}

/* ---------------------------------------------------------------- startup */
#define MAXARGS 128
static char argbuf[MAXARGS][9];
static char *argv_[MAXARGS + 1];

#ifdef CMS_XL
/* XL: called by the loader in AMODE 31 with its services; never returns
   (exit -> _exit -> cms_exit -> the loader's exit) */
void cms_exit(int rc) { xl->exit(rc); for (;;) ; }
int cms_xl_main(struct xlsvc *s)
{
    xl = s;
    L = s->low;
    exit(cms_run_main(s->argc, s->argv));
}
#endif

int cms_main(unsigned char *plist)
{
    int argc = 0;
    while (argc < MAXARGS && plist[0] != 0xFF) {
        int k = 0;
        for (int i = 0; i < 8 && plist[i] != 0x40; i++) {
            char c = rt_e2a[plist[i]];
            if (c >= 'A' && c <= 'Z' && argc == 0) c += 32;   /* program name */
            argbuf[argc][k++] = c;
        }
        argbuf[argc][k] = 0;
        argv_[argc] = argbuf[argc];
        argc++;
        plist += 8;
    }
    argv_[argc] = 0;
    /* CMS cuts every token to 8 characters: an EXEC passes longer
       arguments by stacking them (QUEUE) and giving the token =STACK,
       which is replaced by the words of that stacked line (case kept;
       "..." or '...' groups a word with blanks) */
    for (int j = 1; j < argc; j++) {
        static char sline[4096];
        static char *sargv[MAXARGS + 1];
        char *q;
        int n = 0, len = 0;
        if (strcmp(argv_[j], "=STACK") && strcmp(argv_[j], "=stack")) continue;
        /* a stacked line ending in " \" continues on the next one (a
           terminal line holds 130 characters; compilers need more) */
        for (;;) {
            int k = tread(sline + len, sizeof sline - 1 - len);
            len += k;
            sline[len] = 0;
            if (len && sline[len - 1] == '\n') sline[--len] = 0;
            if (len >= 2 && sline[len - 1] == '\\' && sline[len - 2] == ' ') {
                sline[--len] = 0;
                if (len >= (int)sizeof sline - 132) break;
                continue;
            }
            break;
        }
        if (len && sline[len - 1] == '\n') sline[len - 1] = 0;
        for (int i = 0; i < j; i++) sargv[n++] = argv_[i];
        q = sline;
        while (*q && n < MAXARGS) {
            while (*q == ' ') q++;
            if (!*q) break;
            if (*q == '"' || *q == '\'') {
                char d = *q++;
                sargv[n++] = q;
                while (*q && *q != d) q++;
            } else {
                sargv[n++] = q;
                while (*q && *q != ' ') q++;
            }
            if (*q) *q++ = 0;
        }
        for (int i = j + 1; i < argc && n < MAXARGS; i++) sargv[n++] = argv_[i];
        sargv[n] = 0;
        tin_len = tin_pos = 0;
        exit(cms_run_main(n, sargv));
    }
    exit(cms_run_main(argc, argv_));
}
int _gettimeofday(struct timeval *tv, void *tz) { return gettimeofday(tv, tz); }

/* M8: cREXX's CREXX_CMS_TEXT_IO hooks.  The file is opened as usual; with
 * mainframe_set_text_conversion(0) its records arrive as raw IBM-1047 bytes
 * with ASCII LF between them, and cREXX's own codec decodes them. */
FILE *crexx_cms_text_open(const char *path, const char *mode) { return fopen(path, mode); }
int crexx_cms_text_encoding(const char *encoding) { (void)encoding; return 0; }

/* M8: opendir/readdir/closedir for cREXX's import discovery (CREXX_CMS_DIRENT).
 * A directory is a filemode letter ("A", "a", "a/" or "." for A).  The listing
 * comes from  LISTFILE * * m (EXEC , which writes CMS EXEC A1 with one
 * "&1 &2 FN FT FM" line per file.  Names are returned as "fn.ft". */
struct dirent { unsigned int d_ino; unsigned char d_type; char d_name[20]; };
struct m8_dir { char *names; int count, pos; struct dirent ent; };
struct m8_dir *opendir(const char *name)
{
    unsigned char *pl = L->dirpl;    /* below 16 MB: the stack is not */
    char mode = 'A';
    if (name && name[0] && name[0] != '.') mode = name[0];
    if (mode >= 'a' && mode <= 'z') mode -= 32;
    const char *tok[6] = { "LISTFILE", "*", "*", 0, "(", "EXEC" };
    char m[2] = { mode, 0 };
    tok[3] = m;
    memset(pl, 0x40, sizeof L->dirpl);
    for (int t = 0; t < 6; t++)
        for (int i = 0; tok[t][i] && i < 8; i++) pl[t * 8 + i] = rt_a2e[(unsigned char)tok[t][i]];
    memset(pl + 48, 0xFF, 8);
    int rc = cms202(pl);
    struct m8_dir *d = calloc(1, sizeof *d);
    if (!d) { errno = ENOMEM; return 0; }
    if (rc) return d;                                /* no files: empty */
    struct cfile f;
    memset(&f, 0, sizeof f);
    if (parse_name("CMS EXEC A1", &f)) return d;
    int save = rt_conv; rt_conv = 1;
    int r = read_whole(&f);
    rt_conv = save;
    if (r) { free(f.data); return d; }
    d->names = malloc(f.len / 2 + 64);
    char *out = d->names, *p = f.data, *end = f.data + f.len;
    while (p < end) {
        char *nl = memchr(p, '\n', end - p); if (!nl) nl = end;
        char tk[5][16]; int nt = 0;
        for (char *q = p; q < nl && nt < 5; ) {
            while (q < nl && *q == ' ') q++;
            int k = 0;
            while (q < nl && *q != ' ' && k < 15) tk[nt][k++] = *q++;
            while (q < nl && *q != ' ') q++;
            if (k) { tk[nt][k] = 0; nt++; }
        }
        int b = 0; while (b < nt && tk[b][0] == '&') b++;
        if (nt - b >= 2) {
            int n = sprintf(out, "%s.%s", tk[b], tk[b + 1]);
            for (int i = 0; i < n; i++) if (out[i] >= 'A' && out[i] <= 'Z') out[i] += 32;
            out += n + 1; d->count++;
        }
        p = nl + 1;
    }
    free(f.data);
    return d;
}
struct dirent *readdir(struct m8_dir *d)
{
    if (!d || d->pos >= d->count) return 0;
    char *p = d->names;
    for (int i = 0; i < d->pos; i++) p += strlen(p) + 1;
    d->pos++;
    strncpy(d->ent.d_name, p, sizeof d->ent.d_name - 1);
    d->ent.d_type = 8;
    return &d->ent;
}
int closedir(struct m8_dir *d) { if (d) { free(d->names); free(d); } return 0; }
