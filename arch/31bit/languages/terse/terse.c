/* terse.c -- compress a file into IBM TERSE SPACK format (host mode), in
 * portable C89, for VM/370plus CMS and Linux/390 (9.22).  The output is what
 * TRSMAIN/AMATERSE (SPACK) on z/OS and UNTERSE read back.
 *
 * The encoder runs the decoder's own model -- the pair tree with its
 * reference counts and LRU replacement, as in the Open Mainframe Project's
 * tersedecompress (Apache License 2.0; this file is under the same
 * licence) -- so every code it writes is one the decoder holds at that
 * moment.  It chooses the longest phrase that matches the input.
 *
 *   terse [-b] [-v] [-l lrecl] input output
 *     text (default): lines become EBCDIC records; RECFM F with the longest
 *       line as LRECL (or -l), padded with blanks; -v: RECFM V
 *     -b binary: the bytes as they are, RECFM F, LRECL -l (default 1024)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TREESIZE   0x1000
#define RECORDMARK 257
#define BASE       0
#define CODESIZE   257
#define NONE       (-1)

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

/* ---- the input as symbols: byte+1, RECORDMARK ends a V record ---- */
static int *sym; static long nsym, capsym;
static void addsym(int v)
{
    if (nsym == capsym) {
        capsym = capsym ? capsym * 2 : 65536;
        sym = (int *)realloc(sym, (size_t)capsym * sizeof *sym);
        if (!sym) { fprintf(stderr, "terse: out of storage\n"); exit(12); }
    }
    sym[nsym++] = v;
}

/* ---- output: 12-bit codes ---- */
static FILE *out; static int half = -1;
static void putcode(int c)
{
    if (half < 0) { putc((c >> 4) & 255, out); half = c & 15; }
    else { putc((half << 4) | ((c >> 8) & 15), out); putc(c & 255, out); half = -1; }
}

/* ---- the decoder's model (SpackDecompresser) ---- */
static struct { int Left, Right, Back, NextCount; } Tree[TREESIZE + 1];
static int TreeAvail;
static long nlen[TREESIZE + 1];                 /* expanded length of a node */
static void lruadd(int n)
{
    int back = Tree[BASE].Back;
    Tree[n].NextCount = BASE; Tree[BASE].Back = n; Tree[n].Back = back; Tree[back].NextCount = n;
}
static void unhash(int n);
static void deleteref(int d)
{
    if (Tree[d].NextCount == -1) lruadd(d); else Tree[d].NextCount++;
}
static void lrukill(void)
{
    int p = Tree[0].NextCount, q = Tree[p].NextCount, r = Tree[p].Back;
    Tree[q].Back = r; Tree[r].NextCount = q;
    deleteref(Tree[p].Left); deleteref(Tree[p].Right);
    unhash(p);
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

/* ---- finding phrases: nodes hashed by their first two symbols ---- */
#define HSIZE 4096
static int hhead[HSIZE], hnext[TREESIZE + 1], hkey[TREESIZE + 1], hashed[TREESIZE + 1];
static int stk[TREESIZE + 1];
static int firstsym(int n) { while (n > CODESIZE) n = Tree[n].Left; return n; }
static int secondsym(int n)
{   /* the 2nd symbol of a node of length >= 2 */
    int sp = 0;
    for (;;) {
        while (n > CODESIZE) { stk[sp++] = Tree[n].Right; n = Tree[n].Left; }
        /* n is the first leaf; the next leaf is the first of stk[sp-1] */
        return firstsym(stk[sp - 1]);
    }
}
static int hk(int a, int b) { return (a * 263 + b) & (HSIZE - 1); }
static void dohash(int n)
{
    int k = hk(firstsym(n), secondsym(n));
    hkey[n] = k; hnext[n] = hhead[k]; hhead[k] = n; hashed[n] = 1;
}
static void unhash(int n)
{
    int *pp;
    if (!hashed[n]) return;
    for (pp = &hhead[hkey[n]]; *pp; pp = &hnext[*pp])
        if (*pp == n) { *pp = hnext[n]; break; }
    hashed[n] = 0;
}
/* does node n expand to sym[pos..pos+nlen[n]) ? */
static int matches(int n, long pos)
{
    int sp = 0;
    if (pos + nlen[n] > nsym) return 0;
    for (;;) {
        while (n > CODESIZE) { stk[sp++] = Tree[n].Right; n = Tree[n].Left; }
        if (sym[pos++] != n) return 0;
        if (sp == 0) return 1;
        n = stk[--sp];
    }
}
static int longest(long pos)
{
    int best = sym[pos], n;
    long bl = 1;
    if (pos + 1 < nsym)
        for (n = hhead[hk(sym[pos], sym[pos + 1])]; n; n = hnext[n])
            if (nlen[n] > bl && matches(n, pos)) { best = n; bl = nlen[n]; }
    return best;
}

int main(int argc, char **argv)
{
    int text = 1, varf = 0, a = 1, i, H, G, N;
    long lrecl = 0, pos, maxl = 0;
    unsigned char A2E[256];
    FILE *in;
    while (a < argc && argv[a][0] == '-') {
        if (!strcmp(argv[a], "-b")) text = 0;
        else if (!strcmp(argv[a], "-v")) varf = 1;
        else if (!strcmp(argv[a], "-l") && a + 1 < argc) lrecl = atol(argv[++a]);
        else break;
        a++;
    }
    if (argc - a != 2) { fprintf(stderr, "usage: terse [-b] [-v] [-l lrecl] input output\n"); return 4; }
    if (!text && varf) { fprintf(stderr, "terse: -v is for text\n"); return 4; }
    for (i = 0; i < 256; i++) A2E[EbcToAsc[i]] = (unsigned char)i;
    if (!(in = fopen(argv[a], text ? "r" : "rb"))) { fprintf(stderr, "terse: cannot open %s\n", argv[a]); return 28; }
    if (text) {                                     /* read the lines */
        static char line[65536];
        char **lines = 0; long nl = 0, cap = 0, k, l;
        while (fgets(line, sizeof line, in)) {
            l = (long)strlen(line);
            if (l && line[l - 1] == '\n') line[--l] = 0;
            if (nl == cap) { cap = cap ? cap * 2 : 1024; lines = (char **)realloc(lines, (size_t)cap * sizeof *lines); }
            lines[nl] = (char *)malloc((size_t)l + 1); strcpy(lines[nl++], line);
            if (l > maxl) maxl = l;
        }
        if (!lrecl) lrecl = maxl ? maxl : 1;
        for (k = 0; k < nl; k++) {
            l = (long)strlen(lines[k]);
            if (l > lrecl) l = lrecl;
            for (i = 0; i < l; i++) addsym(A2E[(unsigned char)lines[k][i]] + 1);
            if (varf) { if (k + 1 < nl) addsym(RECORDMARK); }   /* the last ends at EOF */
            else for (; i < lrecl; i++) addsym(0x40 + 1);
        }
    } else {
        int c;
        if (!lrecl) lrecl = 1024;
        while ((c = getc(in)) != EOF) addsym(c + 1);
    }
    fclose(in);
    if (nsym == 0) { fprintf(stderr, "terse: empty input\n"); return 8; }
    if (lrecl > 32760) { fprintf(stderr, "terse: LRECL over 32760\n"); return 8; }
    if (!(out = fopen(argv[a + 1], "wb"))) { fprintf(stderr, "terse: cannot write %s\n", argv[a + 1]); return 28; }
    /* host SPACK header: 05, RECFM flag, LRECL (2), flags, ratio, blksize (2), LRECL (4) */
    putc(0x05, out); putc(varf, out); putc((int)(lrecl >> 8) & 255, out); putc((int)lrecl & 255, out);
    putc(0, out); putc(0, out); putc(0, out); putc(0, out);
    putc(0, out); putc(0, out); putc((int)(lrecl >> 8) & 255, out); putc((int)lrecl & 255, out);
    /* TreeInit, as the decoder */
    for (i = BASE; i <= CODESIZE; i++) { Tree[i].Left = NONE; Tree[i].Right = i; nlen[i] = 1; }
    for (i = CODESIZE + 1; i <= TREESIZE - 1; i++) { Tree[i].NextCount = i + 1; Tree[i].Left = NONE; Tree[i].Right = NONE; }
    Tree[TREESIZE].NextCount = NONE;
    Tree[BASE].NextCount = BASE; Tree[BASE].Back = BASE;
    for (i = 1; i <= CODESIZE; i++) Tree[i].NextCount = NONE;
    TreeAvail = CODESIZE + 1;
    Tree[TREESIZE - 1].NextCount = NONE;
    pos = 0;
    H = longest(pos); putcode(H); pos += nlen[H];
    while (pos < nsym) {
        if (TreeAvail == NONE) lrukill();
        G = longest(pos); putcode(G); pos += nlen[G];
        N = TreeAvail; TreeAvail = Tree[N].NextCount;
        Tree[N].Left = H; Tree[N].Right = G; nlen[N] = nlen[H] + nlen[G];
        bumpref(H); bumpref(G);
        lruadd(N);
        dohash(N);
        H = G;
    }
    putcode(0);                                     /* end of file */
    if (half >= 0) putc(half << 4, out);
    if (fclose(out)) { fprintf(stderr, "terse: write error\n"); return 12; }
    return 0;
}
