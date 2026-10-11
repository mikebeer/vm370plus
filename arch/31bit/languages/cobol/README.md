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

## CMS -- planned

Goal: compile COBOL natively on CMS.
1. Cross route (M5f, GCC 13 -m31, newlib, cmsrt): cobc and libcob as CMS
   MODULE/TXTLIB, to prove they run under CMS.
2. Native: cobc, libcob and GMP compiled on CMS by GCC380 against GCCLIB31,
   with a `COBOL EXEC`: `cobc -C` -> `GCC` -> `LOAD (+libcob TXTLIB)` ->
   `GENMOD`.
