# PL/I for VM/370plus (9.20): mikebeer/pli

`mikebeer/pli` translates PL/I to C89. Its run-time is `plirt.h`, which is
header only and all `static`.

## CMS: natively compiled (12 October 2026)

    PLIC fn      fn PLI -> fn C (PLI2C MODULE) -> GCC380 + GCCLIB31 -> fn MODULE

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
