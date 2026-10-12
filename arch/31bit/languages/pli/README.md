# PL/I for VM/370plus (9.20): mikebeer/pli

`mikebeer/pli` translates PL/I to C89. Its run-time is `plirt.h`, which is
header only and all `static`.

## CMS: natively compiled (12 October 2026)

    PLIC fn            fn PLI -> fn C (PLI2C MODULE) -> CC EXEC (GCC 13) -> fn MODULE
    PLIC fn ( GCC380   the same through GCC380 and GCCLIB31

**Through GCC 13 (tests tc22-tc24): all 19 upstream tests give the PC's
output.** IEEE floating point fixed T02; a line `/*` on the stack is end of
file for SYSIN (cmsrt), which T11 and T20 need. PLICMS H maps PL/I file
names to CMS ids for both compilers; a name part longer than 8 becomes its
first and last 4 letters and digits (`plitest_out.txt` -> `PLITTOUT TXT`),
so `plitest_out` and `plitest_rec` no longer collide (T12).

The first route, GCC380 (below), is kept as `( GCC380`.

- **PLI2C MODULE** is the translator, cross-built through M5f (`m5f/crt.sh pli_all.c`).
- **The C it writes is compiled on CMS by GCC380.** Two things make that
  possible:
  - plirt.h uses no `long long`, so GCC380's internal error on 64-bit
    addition does not arise (it blocks libcob, 9.19);
  - everything is `static`, so the 8-character external names do not
    matter.
- **`cms/PLICMS.H`** is the prelude to plirt.h on CMS (`PLIRT H` begins with
  `#include "plicms.h"`). It does two things:
  - maps PL/I TITLE and DD names (`plitest_out.txt`) to CMS file ids
    (`PLITEST TXT A`), because GCCLIB31's fopen takes `FN FT FM`;
  - corrects GCCLIB31's `getc(s)` macro, which expands to `fgetc((c))`.
- **`mkcmspli.py`** builds the decks: PLI2C TEXT, plus PLIC EXEC, PLIRT H,
  the tests and PLITEST EXEC.

Tests pp1–pp3: 17 of the 19 upstream tests give the PC's output on CMS.
- **T02:** HFP doubles print one more, noisy, digit (`3.230769230769235` against
  `3.23076923076923`, `3.400000000000004` against `3.4`).
- **T11:** list-directed GET from SYSIN reads only the first stacked line,
  then waits at the terminal. The edit-directed GET of T19 reads its stacked
  lines.

On CMS, SYSIN is the terminal and the console stack.

## Debian (12 October 2026, test pd1)

    plic fn.pli      fn.pli -> fn.c (pli) -> gcc 4.6 -> ./fn

The translator (`pli_all.c`) compiles on the Debian guest with its own gcc
4.6 (about 35 minutes under emulation, once); `tests/run.sh` then gives
**19 of 19** tests identical to the PC. Installed on the Debian disk:
`/usr/local/bin/pli`, `/usr/local/bin/plic` and `/usr/local/include/plirt.h`.
