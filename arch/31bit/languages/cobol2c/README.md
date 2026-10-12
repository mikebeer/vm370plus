# COBOL with OO -- mikebeer/cobol on VM/370plus

`github.com/mikebeer/cobol` translates COBOL-85 (plus OO: CLASS-ID,
METHOD-ID, INVOKE; indexed files; SORT; free format; user functions) into
C89.  Its run-time is one header, `cobrt.h`.

## CMS

| file          | what                                                        |
|---------------|-------------------------------------------------------------|
| COB2C MODULE  | the translator (`cobol_all.c` by m5f/cmscc, elf_to_cms, GENMOD) |
| COBRT H       | the run-time header (V format; lines up to 80)              |
| COBX EXEC     | `COBX fn [( C]`: fn COB -> fn C -> CC EXEC (GCC 13) -> fn MODULE |
| COBXTEST EXEC | `COBXTEST T01 T02 ...`: translate, compile and run each test |

COPY books are `name CPY`; test input is `fn IN`.

## Debian

The translator is cross-compiled with `../common/wcc` (GCC 13 `-m31`
against the Debian 7 sysroot) and installed as `/usr/local/bin/cobol`,
with `cobrt.h` in `/usr/local/share/cobol`; the guest's own gcc compiles
the C it writes.

## Tests

The 20 upstream tests (`tests/t01_hello` ... `t20_recur`).
