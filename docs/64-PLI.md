# PL/I for VM/370plus (9.20, research, 11 October 2026)

The owner's question: VM/370 has an old PL/I. Is there a newer one for CMS
and for the Debian guest?

## What VM/370plus already has

CE ships an IBM PL/I compiler on the Y disk (19E): `PLI MODULE`,
`PLILIB TXTLIB`, `PLIDIRT TEXT`. The note about it is `PLICOMP MEMO` (U disk),
and there is a sample, `HELLO PLI` (U disk).

Tests pl1 and pl2 (Hercules 4, IPL CMS, 16 MB):
- `PLI HELLO` compiles cleanly, producing HELLO TEXT and HELLO LISTING.
- `LOAD HELLO (START` without the library leaves `IHESADA ... IHEERRC`
  unresolved. The library has to be named first:

      GLOBAL TXTLIB PLILIB
      LOAD HELLO (START

  With that, pl2 printed `The rabbit squeaks to the world.`

This is the PL/I(F) language level of the 1960s/70s (for example no
`SELECT` group or `LEAVE`, which came with the later Optimizing compiler),
with the IHE runtime. It runs in 24-bit
mode under CMS's OS simulation.

## Newer PL/I implementations (survey)

| Implementation | State | Usable here? |
|---|---|---|
| **Iron Spring PL/I** (Peter Flass) | Active: 1.4.0 in May 2025, preprocessor 0.3.0 in Sep 2025. Linux and OS/2. Compiler freeware, closed source; runtime library LGPL 2.1 | No. The compiler is an x86 binary and generates x86 code, so it cannot run on, or target, s390/CMS without its source |
| **pl1gcc** (GCC front end, SourceForge) | Abandoned in the mid-2000s; it parsed PL/I but never generated working code | No |
| **PLI-2000** (Steadsoft, GPL-3.0) | Shelved; Windows NT, writes x86 COFF objects; "much of" ANSI X3.74-1987 | Only as a reference: its front end and C I/O library could seed a translator |
| **Multics PL/I** | Historic; written in PL/I for the Multics 6180 | No |
| IBM Enterprise PL/I | Current, z/OS only, licensed | No |

No maintained, open PL/I targets s390 or produces C.

## Options

1. **Document and polish CE's PL/I** (small):
   - a `PLI` EXEC that sets `GLOBAL TXTLIB PLILIB` itself;
   - a help page;
   - a regression test in the run set.
2. **A PL/I subset-to-C translator** (large, new development). It would cover
   roughly ANSI PL/I subset G (X3.74):
   - `PROC`/`BEGIN`, `DO`/`SELECT`;
   - fixed decimal and binary, `CHAR VARYING`, `PICTURE`;
   - structures and arrays, `BASED`/`POINTER`;
   - `ON` conditions;
   - `STREAM` and `RECORD` I/O.

   It would be written in C (or PL/I-in-C89), generate C, and reuse the
   same toolchains as COBOL: gcc on Debian, and the CMS route of 9.19.
   Decimal arithmetic could come from GMP, as libcob does.
3. **Wait for a native C compiler on CMS** (the 9.19 decision). Option 2
   only becomes "native on CMS" once there is a C compiler there that can
   handle 64-bit integers (GCC380 cannot: internal error on `long long`
   addition).

Recommendation: option 1 now (an hour's work), and option 2 after the 9.19
native-compiler decision, as a separate milestone.

Sources: [Iron Spring PL/I news](https://iron-spring.com/news.html),
[Iron Spring PL/I on EDM2](https://www.edm2.com/index.php/Iron_Spring_PL/I),
[pl1gcc](https://sourceforge.net/p/pl1gcc/),
[PLI-2000](https://github.com/Steadsoft/PLI-2000),
[The Multics PL/1 Compiler](https://www.multicians.org/pl1-raf.html).
