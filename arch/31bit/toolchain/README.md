# C/C++ toolchain on CMS (9.21)

GCC 13.3 (`cc1`, `cc1plus`) and binutils 2.42 (`as`, `ld`, `ar`) run on
VM/370plus CMS. They are cross-built by `mktoolchain.sh` with `m5f/cmscc`
and `m5f/cmsc++` for the M5f runtime (newlib + cmsrt, AMODE 31, ASCII inside,
EBCDIC at the CMS boundary).

**First native build, 12 October 2026, test tc15:** hello.c was compiled by
cc1, assembled by as, linked by ld, turned into a TEXT deck by elf2cms,
LOADed and GENMODed on CMS. It prints, and returns RC 3.

**C++, 12 October 2026, tests tc20/tc21** (`CP DEFINE STORAGE 256M`,
Hercules MAINSIZE 256): `CC CPPTEST CPP` compiles natively (107 s CPU) and
CPPTEST runs (classes, virtual functions, templates, unique_ptr, vector,
map, sort, a static constructor). iostreams (`tests/iostest.cpp`) run.
With MAINSIZE 16 a C++ compile pages for many minutes (and the LOGOFF of
that machine hung CP: I-272).

**COBOL natively** (9.19): `COBOL HELLO` = cobc on CMS + CC EXEC with
libcob from SYSROOT TAR (tc17).

## Pieces

| On CMS | What | How it runs |
|---|---|---|
| CC1 XIMAGE (23 MB), CC1PLUS XIMAGE | GCC 13 compilers | `XL CC1 ...`: the XL loader (XL MODULE) runs them from high storage |
| AS, LD, AR MODULE | GNU binutils | ordinary modules (on an EDF disk since the I-271 fix) |
| ELF2CMS XIMAGE | ELF -> TEXT deck (elf_to_cms) | `XL ELF2CMS in out` |
| SYSROOT TAR | /usr/include (newlib, m5f, gcc), /usr/lib (libc, libm, libsoftfp, libgcc, cmsrt.o, entry31.o, image.ld) | cmsrt resolves absolute paths in it, read-only |
| CXXROOT TAR | /usr/include/c++ (libstdc++ 13.3), /usr/lib/cxx (libstdc++.a, cxxrt.o) | read only for C++ paths, so a C compile does not hold it in storage |
| CC EXEC | the driver: `CC fn` -> fn MODULE; `CC fn CPP` for C++ | cc1/cc1plus -> as -> ld -> elf2cms -> LOAD -> GENMOD |

## What the runtime needed (m5f)

- **The XL loader** (`xl.c`, `mkximage.py`, `entryxl.s`, `cmsrt -DCMS_XL`):
  programs above 16 MB. The `.got` words are relocated too, since libgcc
  reaches `__clz_tab` through the GOT.
- **The sysroot VFS:** absolute paths are looked up in SYSROOT TAR.
- **Distinct `st_ino`** (a hash of the file id or path). GCC drops include
  directories that share (dev, ino), and elf_to_cms refuses to write its
  output when the output looks like its input.
- **POSIX names** with the reentrant `errno`, plus `realpath`. libiberty's
  `lrealpath` returns garbage without realpath, and its callers free the
  argument string.
- **Others:**
  - `mkstemp` makes CMS file ids;
  - binary file types (O, A, ELF, TMP, TAR...);
  - TEXT is written F 80;
  - `=STACK` lines ending in ` \` continue, so command lines can exceed 130
    characters;
  - C++ static constructors;
  - `cxxrt.cc`.

## C++ library

`mklibstdcxx.sh` builds libstdc++ 13.3 from the GCC source for this target:
newlib, `-m31 -msoft-float`, the generic "C" locale, one thread. Ubuntu's
32-bit s390 libstdc++ was tried first and does not fit: it is built for
glibc (its own `stdout`, glibc's locale structures) and for hard float, so
doubles crossing between it and newlib or the program were garbage (3.5
printed as 1.4982e-308). `cxxrt.cc` supplies operator new/delete, the
guards, and stubs that make a `throw` end the program (no unwind tables).

newlib was built with `__SINGLE_THREAD__` (FILE is 100 bytes); cmscc and
CC EXEC define it too.

## Getting files onto CMS

The decks are large (CC1PLUS XIMAGE 25 MB, CXXROOT TAR 15 MB), and a run
of them through the card reader overflowed the spool (tc18). With Hercules
down, `tools/edf.py IMAGE putlist LIST --cckd` writes them straight onto
the EDF disk (181 = edf2.cckd): `BIN` files as F 80 records, as READCARD
gives them, text in IBM-1047 as Hercules converts it. 8 seconds.

## Open

- `ld @file` response files (cmsrt's read of the response file).
- Exceptions (would need unwind tables kept by elf2cms and libgcc_eh).
- cc1/cc1plus are still linked with Ubuntu's libstdc++ (they work; relink
  with the new one at the next full build).
- Speed: about 34 s of CPU for a small C program, 1-2 min for C++.
