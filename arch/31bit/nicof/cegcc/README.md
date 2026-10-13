# NICOF's socket client, built with CE's GCC (9.5)

NICOF's own `NICOFLIB TXTLIB` was compiled against an older GCCLIB than
CE's. Linked with CE's GCCLIB, the first `send()` of a program works and
the second fails with `VMCF(8) - duplicate message`, and an empty request
once ended in `Auxiliary stack overflow`. These are the client parts
rebuilt with CE's compiler, so that CE C programs (HTTPD) get sockets:

| File | |
|---|---|
| `nicofclt.c`, `ncfsockt.c` | NICOF 0.7.0 sources (public domain), unchanged apart from LF line ends |
| `INTRAPI.ASSEMBLE` | NICOF's interrupt layer. Its external and device interrupt handlers now build a CE GCCLIB frame: DSTACK at +0 (the end of the stack, flags 0), GCCCRABA at +72 (taken from the caller of `enable_ext`) and STACKNXT at +76. The original used the older layout (CONFILOU, GCCCRAB). |

## Build on CMS

The compiler, GCC380, takes an operation exception under `IPL CMS`, so
build under `IPL 290` with `DEF STOR 64M`:

    NCFACC                                   NICOF's headers on N
    GLOBAL MACLIB DMKMAC CMSLIB OSMACRO
    ASSEMBLE INTRAPI
    GCC NICOFCLT ( CSECT CMS
    GCC NCFSOCKT ( CSECT CMS
    GCC HTTPD ( CSECT CMS
    GLOBAL TXTLIB GCCLIB
    LOAD HTTPD NICOFCLT NCFSOCKT INTRAPI (CLEAR
    GENMOD HTTPD

It runs under `IPL CMS` and under `IPL 290` (16M or 64M).

## The EC-mode CMS (I-285)

NICOF's C interrupt handler reads the interrupt code from bytes 2-3 of the
external old PSW that DMSITE saves (user area +96). Under `IPL 290` that PSW
is an EC PSW, which has no code in it: the code is at X'86'. Before the fix
the handler never saw the VMCF code X'4001', so the first socket call
waited forever. INTRAPI's EXTHNDLR now gives the C handler a copy of the
user area and, for an EC old PSW, puts X'86' into the copy's bytes 2-3.

Do not end a NICOF program with HX: its HNDEXT trap stays set and points
into the freed program, and the next external interrupt ends in a disabled
wait. Use `HTTPD -n` or `GET /STOP`, and re-IPL CMS after an HX.

## Two things CE's C does differently

- **Newlines.** `'\n'` is EBCDIC NL (X'15'), and NICOF's translation table
  turns it into a blank. HTTPD translates `'\n'` to ASCII LF itself and
  finds the end of a request in the ASCII bytes.
- **`sscanf`.** `%7s` left the field empty, so HTTPD parses by hand.

NICOF moves at most 2 KB per socket call, so HTTPD sends and receives in
2,000-byte pieces. Text goes out through a buffer, a block per call and not
a line per call: a 9 KB file now takes 0.6 s instead of 26 s.
