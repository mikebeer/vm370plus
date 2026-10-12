/* unterse.c -- decompress IBM TERSE files (PACK and SPACK, host and native
 * modes) in portable C89, for VM/370plus CMS and Linux/390 (9.22).
 *
 * A C rendering of the Open Mainframe Project's tersedecompress (Java,
 * Apache License 2.0, github.com/openmainframeproject/tersedecompress),
 * itself derived from IBM's TERSE decompression code; this file is under
 * the same licence.
 *
 *   unterse [-b] input output
 *     text mode (default): EBCDIC records become lines of text (ASCII here;
 *       on CMS the runtime writes them back as EBCDIC 1047 text records)
 *     -b binary mode: the record bytes unchanged; variable-length records
 *       get a 4-byte RDW each, as on z/OS
 *   unterse -i input   show the header
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TREESIZE   0x1000
#define STACKSIZE  0x07FF
#define RECORDMARK 257
#define BASE       0
#define CODESIZE   257
#define ENDOFFILE  0
#define NONE       (-1)
#define FLAGMVS    0x04

#ifdef __CMS__
/* IBM-1047, the inverse of the CMS runtime's ASCII -> EBCDIC table, so a
   text file comes back to CMS byte for byte */
static const unsigned char EbcToAsc[256] = {
    0x00,0x01,0x02,0x03,0x9C,0x09,0x86,0x7F,0x97,0x8D,0x8E,0x0B,0x0C,0x0D,0x0E,0x0F,
    0x10,0x11,0x12,0x13,0x9D,0x85,0x08,0x87,0x18,0x19,0x92,0x8F,0x1C,0x1D,0x1E,0x1F,
    0x80,0x81,0x82,0x83,0x84,0x0A,0x17,0x1B,0x88,0x89,0x8A,0x8B,0x8C,0x05,0x06,0x07,
    0x90,0x91,0x16,0x93,0x94,0x95,0x96,0x04,0x98,0x99,0x9A,0x9B,0x14,0x15,0x9E,0x1A,
    0x20,0xA0,0xE2,0xE4,0xE0,0xE1,0xE3,0xE5,0xE7,0xF1,0xA2,0x2E,0x3C,0x28,0x2B,0x7C,
    0x26,0xE9,0xEA,0xEB,0xE8,0xED,0xEE,0xEF,0xEC,0xDF,0x21,0x24,0x2A,0x29,0x3B,0x5E,
    0x2D,0x2F,0xC2,0xC4,0xC0,0xC1,0xC3,0xC5,0xC7,0xD1,0xA6,0x2C,0x25,0x5F,0x3E,0x3F,
    0xF8,0xC9,0xCA,0xCB,0xC8,0xCD,0xCE,0xCF,0xCC,0x60,0x3A,0x23,0x40,0x27,0x3D,0x22,
    0xD8,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0xAB,0xBB,0xF0,0xFD,0xFE,0xB1,
    0xB0,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,0x70,0x71,0x72,0xAA,0xBA,0xE6,0xB8,0xC6,0xA4,
    0xB5,0x7E,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0xA1,0xBF,0xD0,0x5B,0xDE,0xAE,
    0xAC,0xA3,0xA5,0xB7,0xA9,0xA7,0xB6,0xBC,0xBD,0xBE,0xDD,0xA8,0xAF,0x5D,0xB4,0xD7,
    0x7B,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0xAD,0xF4,0xF6,0xF2,0xF3,0xF5,
    0x7D,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,0x50,0x51,0x52,0xB9,0xFB,0xFC,0xF9,0xFA,0xFF,
    0x5C,0xF7,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0xB2,0xD4,0xD6,0xD2,0xD3,0xD5,
    0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0xB3,0xDB,0xDC,0xD9,0xDA,0x9F
};
#else
static const unsigned char EbcToAsc[256] = {
    0x00,0x01,0x02,0x03,0xCF,0x09,0xD3,0x7F,0xD4,0xD5,0xC3,0x0B,0x0C,0x0D,0x0E,0x0F,
    0x10,0x11,0x12,0x13,0xC7,0xB4,0x08,0xC9,0x18,0x19,0xCC,0xCD,0x83,0x1D,0xD2,0x1F,
    0x81,0x82,0x1C,0x84,0x86,0x0A,0x17,0x1B,0x89,0x91,0x92,0x95,0xA2,0x05,0x06,0x07,
    0xE0,0xEE,0x16,0xE5,0xD0,0x1E,0xEA,0x04,0x8A,0xF6,0xC6,0xC2,0x14,0x15,0xC1,0x1A,
    0x20,0xA6,0xE1,0x80,0xEB,0x90,0x9F,0xE2,0xAB,0x8B,0x9B,0x2E,0x3C,0x28,0x2B,0x7C,
    0x26,0xA9,0xAA,0x9C,0xDB,0xA5,0x99,0xE3,0xA8,0x9E,0x21,0x24,0x2A,0x29,0x3B,0x5E,
    0x2D,0x2F,0xDF,0xDC,0x9A,0xDD,0xDE,0x98,0x9D,0xAC,0xBA,0x2C,0x25,0x5F,0x3E,0x3F,
    0xD7,0x88,0x94,0xB0,0xB1,0xB2,0xFC,0xD6,0xFB,0x60,0x3A,0x23,0x40,0x27,0x3D,0x22,
    0xF8,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x96,0xA4,0xF3,0xAF,0xAE,0xC5,
    0x8C,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,0x70,0x71,0x72,0x97,0x87,0xCE,0x93,0xF1,0xFE,
    0xC8,0x7E,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0xEF,0xC0,0xDA,0x5B,0xF2,0xF9,
    0xB5,0xB6,0xFD,0xB7,0xB8,0xB9,0xE6,0xBB,0xBC,0xBD,0x8D,0xD9,0xBF,0x5D,0xD8,0xC4,
    0x7B,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0xCB,0xCA,0xBE,0xE8,0xEC,0xED,
    0x7D,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,0x50,0x51,0x52,0xA1,0xAD,0xF5,0xF4,0xA3,0x8F,
    0x5C,0xE7,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0xA0,0x85,0x8E,0xE9,0xE4,0xD1,
    0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0xB3,0xF7,0xF0,0xFA,0xA7,0xFF
};
#endif

/* ---- input: 12-bit codes ---- */
static FILE *in;
static int bitsavail, savedbits;
static int getblok(void)
{
    int b1, b2;
    if (bitsavail == 0) {
        if ((b1 = getc(in)) == EOF) return ENDOFFILE;
        if ((b2 = getc(in)) == EOF) { fprintf(stderr, "unterse: truncated input\n"); exit(8); }
        savedbits = b2 & 0x0F; bitsavail = 4;
        return (b1 << 4) | (b2 >> 4);
    }
    if ((b2 = getc(in)) == EOF) return ENDOFFILE;   /* the half byte was padding */
    bitsavail = 0;
    return (savedbits << 8) | b2;
}

/* ---- output: the four record formats ---- */
static FILE *out;
static int text, recfmv, lrecl, recbytes;
static unsigned char *recbuf;
static void endrec(void)
{
    if (text) { putc('\n', out); recbytes = 0; return; }
    if (recfmv) {                                    /* binary V: RDW + data */
        int len = recbytes + 4;
        putc((len >> 8) & 255, out); putc(len & 255, out); putc(0, out); putc(0, out);
        fwrite(recbuf, 1, (size_t)recbytes, out);
    }
    recbytes = 0;
}
static void putchr(int x)
{
    if (recfmv) {
        if (x == 0 || x == RECORDMARK) {
            if (text || x == RECORDMARK) endrec();
            return;
        }
        if (text) putc(EbcToAsc[x - 1], out);
        else if (recbytes < lrecl) recbuf[recbytes++] = (unsigned char)(x - 1);
        return;
    }
    if (x == 0) return;
    if (text) {
        putc(EbcToAsc[x - 1], out);
        if (++recbytes == lrecl) endrec();
    } else putc(x - 1, out);
}
static void closeout(void)
{
    if (text) { if (recfmv || recbytes > 0) endrec(); }
    else if (recfmv && recbytes > 0) endrec();
}

/* ---- PACK ---- */
static int Father[TREESIZE], CharExt[TREESIZE], Backward[TREESIZE], Forward[TREESIZE];
static void unpack(void)
{
    int H1, H2, x, d, y, q, r, e, p, h;
    H2 = 1 + 0x40;                                    /* EBCDIC blank */
    for (H1 = 258; H1 < 4096; H1++) { Father[H1] = H2; CharExt[H1] = 1 + 0x40; H2 = H1; }
    for (H1 = 258; H1 < 4095; H1++) { Backward[H1 + 1] = H1; Forward[H1] = H1 + 1; }
    Backward[0] = 4095; Forward[0] = 258; Backward[258] = 0; Forward[4095] = 0;
    x = 0;
    d = getblok();
    while (d != ENDOFFILE) {
        y = Backward[0]; q = Backward[y]; Backward[0] = q; Forward[q] = 0;
        h = y; p = 0;
        while (d > 257) {
            q = Forward[d]; r = Backward[d]; Forward[r] = q; Backward[q] = r;
            Forward[d] = h; Backward[h] = d; h = d;
            e = Father[d]; Father[d] = p; p = d; d = e;
        }
        q = Forward[0]; Forward[y] = q; Backward[q] = y; Forward[0] = h; Backward[h] = 0;
        CharExt[x] = d;
        putchr(d);
        x = y;
        while (p != 0) { e = Father[p]; putchr(CharExt[p]); Father[p] = d; d = p; p = e; }
        Father[y] = d;
        d = getblok();
    }
}

/* ---- SPACK ---- */
static struct { int Left, Right, Back, NextCount; } Tree[TREESIZE + 1];
static int Stack[STACKSIZE + 1], TreeAvail;
static void putchars(int X)
{
    int head = 0;
    for (;;) {
        while (X > CODESIZE) {
            if (head >= STACKSIZE) { fprintf(stderr, "unterse: stack overflow\n"); exit(8); }
            Stack[++head] = Tree[X].Right; X = Tree[X].Left;
        }
        putchr(X);
        if (head > 0) X = Stack[head--]; else break;
    }
}
static void lruadd(int n)
{
    int back = Tree[BASE].Back;
    Tree[n].NextCount = BASE; Tree[BASE].Back = n; Tree[n].Back = back; Tree[back].NextCount = n;
}
static void deleteref(int d)
{
    if (Tree[d].NextCount == -1) lruadd(d); else Tree[d].NextCount++;
}
static void lrukill(void)
{
    int p = Tree[0].NextCount, q = Tree[p].NextCount, r = Tree[p].Back;
    Tree[q].Back = r; Tree[r].NextCount = q;
    deleteref(Tree[p].Left); deleteref(Tree[p].Right);
    Tree[p].NextCount = TreeAvail; TreeAvail = p;
}
static void bumpref(int b)
{
    if (Tree[b].NextCount < 0) Tree[b].NextCount--;
    else {
        int f = Tree[b].NextCount, p = Tree[b].Back;
        Tree[p].NextCount = f; Tree[f].Back = p; Tree[b].NextCount = -1;
    }
}
static void unspack(void)
{
    int i, N, G, H;
    for (i = BASE; i <= CODESIZE; i++) { Tree[i].Left = NONE; Tree[i].Right = i; }
    for (i = CODESIZE + 1; i <= TREESIZE - 1; i++) {
        Tree[i].NextCount = i + 1; Tree[i].Left = NONE; Tree[i].Right = NONE;
    }
    Tree[TREESIZE].NextCount = NONE;
    Tree[BASE].NextCount = BASE; Tree[BASE].Back = BASE;
    for (i = 1; i <= CODESIZE; i++) Tree[i].NextCount = NONE;
    TreeAvail = CODESIZE + 1;
    Tree[TREESIZE - 1].NextCount = NONE;
    H = getblok();
    if (H == ENDOFFILE) return;
    putchars(H);
    G = getblok();
    while (G != ENDOFFILE) {
        if (TreeAvail == NONE) lrukill();
        putchars(G);
        N = TreeAvail; TreeAvail = Tree[N].NextCount;
        Tree[N].Left = H; Tree[N].Right = G;
        bumpref(H); bumpref(G);
        lruadd(N);
        H = G;
        G = getblok();
    }
}

/* ---- header ---- */
static int rd(int n)
{
    int v = 0, c;
    while (n-- > 0) {
        if ((c = getc(in)) == EOF) { fprintf(stderr, "unterse: short header\n"); exit(8); }
        v = (v << 8) | c;
    }
    return v;
}
int main(int argc, char **argv)
{
    int ver, varflag = 0, rl1 = 0, rl2 = 0, flags = 0, ratio = 0, bsize = 0, host, spack, info = 0;
    int a = 1;
    text = 1;
    while (a < argc && argv[a][0] == '-') {
        if (!strcmp(argv[a], "-b")) text = 0;
        else if (!strcmp(argv[a], "-i")) info = 1;
        else { fprintf(stderr, "usage: unterse [-b] input output | unterse -i input\n"); return 4; }
        a++;
    }
    if (argc - a != (info ? 1 : 2)) { fprintf(stderr, "usage: unterse [-b] input output | unterse -i input\n"); return 4; }
    if (!(in = fopen(argv[a], "rb"))) { fprintf(stderr, "unterse: cannot open %s\n", argv[a]); return 28; }
    ver = rd(1);
    switch (ver) {
    case 0x01: case 0x07:                             /* native: 4-byte header */
        if (rd(3) != 0x8969A5) { fprintf(stderr, "unterse: invalid header\n"); return 8; }
        rl1 = rd(2); host = 0; text = 0; spack = 1;
        lrecl = rl1;
        break;
    case 0x02: case 0x05:                             /* host PACK / SPACK: 12 bytes */
        varflag = rd(1); rl1 = rd(2); flags = rd(1); ratio = rd(1); bsize = rd(2); rl2 = rd(4);
        if (varflag != 0 && varflag != 1) { fprintf(stderr, "unterse: unknown record format flag %x\n", varflag); return 8; }
        if (rl1 == 0 && rl2 == 0) { fprintf(stderr, "unterse: record length is 0\n"); return 8; }
        if (rl1 && rl2 && rl1 != rl2) { fprintf(stderr, "unterse: ambiguous record length\n"); return 8; }
        lrecl = rl1 ? rl1 : rl2; recfmv = varflag == 1; host = 1; spack = ver == 0x05;
        if (!(flags & FLAGMVS) && (flags || ratio || bsize)) { fprintf(stderr, "unterse: bad non-MVS header\n"); return 8; }
        break;
    default:
        fprintf(stderr, "unterse: not a TERSE file (header %02x)\n", ver); return 8;
    }
    if (info) {
        printf("%s %s, RECFM %s, LRECL %d%s\n", host ? "host" : "native", spack ? "SPACK" : "PACK",
               recfmv ? "V" : "F", lrecl, flags & FLAGMVS ? ", from MVS" : "");
        return 0;
    }
    (void)host;
    recbuf = (unsigned char *)malloc((size_t)lrecl + 8);
    if (!(out = fopen(argv[a + 1], text ? "w" : "wb"))) { fprintf(stderr, "unterse: cannot write %s\n", argv[a + 1]); return 28; }
    if (spack) unspack(); else unpack();
    closeout();
    if (fclose(out)) { fprintf(stderr, "unterse: write error\n"); return 12; }
    fclose(in);
    return 0;
}
