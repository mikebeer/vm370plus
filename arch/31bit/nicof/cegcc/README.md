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

Run it under `IPL CMS` (16M). Under `IPL 290`, the first socket call
waited forever (I-285).

## Two things CE's C does differently

- **Newlines.** `'\n'` is EBCDIC NL (X'15'), and NICOF's translation table
  turns it into a blank. HTTPD translates `'\n'` to ASCII LF itself and
  finds the end of a request in the ASCII bytes.
- **`sscanf`.** `%7s` left the field empty, so HTTPD parses by hand.

NICOF moves at most 2 KB per socket call, so HTTPD sends and receives in
2,000-byte pieces.
