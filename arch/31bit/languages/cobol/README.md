# GnuCOBOL for VM/370plus (milestone 9.19)

## Debian 7 (wheezy) s390 -- done 11 October 2026

`mkcobol-linux.sh WORK OUT` cross-builds GnuCOBOL 3.1.2 (cobc, cobcrun,
static libcob) with GMP 6.3 for Linux/390 31-bit against wheezy's own glibc
2.13, giving `gnucobol-3.1.2-s390.tgz` (2.6 MB, also in `dist/`). Install on
Debian:

    cd / && tar xzf gnucobol-3.1.2-s390.tgz
    cobc -x prog.cob && ./prog

cobc translates COBOL to C and calls Debian's gcc 4.6 (the -Dev packages,
`debian/INSTALL-DEV.txt`) with `-lcob -lgmp`. A small program takes about
30 s to compile under VM/370plus.

Verified (test cb1, Debian booted by `CP IPL LINUX`, SSH): the programs in
`tests/` give output identical to GnuCOBOL 3.1.2 on the PC (`*.pc`):
packed and binary arithmetic up to 30 digits, ROUNDED, editing, DIVIDE
REMAINDER, INSPECT/STRING/UNSTRING, OCCURS with SEARCH ALL, intrinsic
functions (PI, EXP, LOG, SIN, RANDOM, dates, NUMVAL), line-sequential and
relative files with REWRITE, SORT USING/GIVING, CALL of a second program.

Not included: INDEXED files (no Berkeley DB), SCREEN SECTION (no curses),
XML/JSON. `2 ** 90` gives 0 on the PC as well (cobc's constant folding).

Cross-building pitfall: configure cannot run its type-size tests and writes
`#define COB_LI_IS_LL $COB_LI_IS_LL`; the sources test it with `#ifdef`, so
libcob treated 32-bit longs as 64-bit and 10-19 digit values came out as 0.
The script removes the define and sets `COB_32_BIT_LONG`.

## CMS, step 1: cross route -- done 11 October 2026

`mkcobol-cms.sh WORK` builds, with the M5f toolchain (GCC 13 -m31,
newlib, cmsrt) through the new gcc-style driver `m5f/cmscc` (so autoconf
projects configure for CMS):
- GMP 6.3 and libcob for CMS;
- `cms/cmsdl.c` (dlopen stubs: CALL is static, `cobc -fstatic-call`) and
  `cms/cobcms.c` (the POSIX names libcob uses: open/read/lseek/...;
  files libcob opens with open() -- SEQUENTIAL, RELATIVE, SORT work files
  -- are binary CMS files, LINE SEQUENTIAL ones are text, ASCII<->EBCDIC);
- a PC cobc for the target (big-endian, 32-bit long). A plain PC cobc
  generates little-endian code for BINARY/COMP fields (SUM 1..1000 came
  out as 467860).

`cobccms prog.cob [sub.cob ...]` gives `prog.text`; on CMS (IPL 290,
Hercules 4): `LOAD prog (CLEAR`, `GENMOD prog`, `prog`.

Verified (tests cc2, cc3): hello, mul, pow, files and t1+subp give the PC's
output on CMS, except FUNCTION RANDOM (it uses the C library's rand(),
newlib's here). File names map as cmsrt does: `"rel.dat"` is REL DAT A.
cmsrt now keeps a file's contents when it is opened for writing without
O_TRUNC (OPEN I-O, EXTEND, "r+").

## CMS, step 2: native -- next

Goal: compile COBOL natively on CMS.
- cobc itself as a CMS MODULE (it only has to translate: `cobc -C`), with
  its generated header names mapped to CMS file ids.
- The generated C compiled on CMS by GCC380 against libcob and GMP built
  with GCC380/GCCLIB31, driven by a `COBOL EXEC`: `cobc -C` -> `GCC` ->
  `LOAD (+ libcob TXTLIB)` -> `GENMOD`.
