# C/C++ toolchain on CMS (9.21)

GCC 13.3 (`cc1`, `cc1plus`) and binutils 2.42 (`as`, `ld`, `ar`) run on
VM/370plus CMS. They are cross-built by `mktoolchain.sh` with `m5f/cmscc`
and `m5f/cmsc++` for the M5f runtime (newlib + cmsrt, AMODE 31, ASCII inside,
EBCDIC at the CMS boundary).

**First native build, 12 October 2026, test tc15:** hello.c was compiled by
cc1, assembled by as, linked by ld, turned into a TEXT deck by elf2cms,
LOADed and GENMODed on CMS. It prints, and returns RC 3.

## Pieces

| On CMS | What | How it runs |
|---|---|---|
| CC1 XIMAGE (23 MB), CC1PLUS XIMAGE | GCC 13 compilers | `XL CC1 ...`: the XL loader (XL MODULE) runs them from high storage |
| AS, LD, AR MODULE | GNU binutils | ordinary modules, kept on an 800-byte disk (I-271) |
| ELF2CMS XIMAGE | ELF -> TEXT deck (elf_to_cms) | `XL ELF2CMS in out` |
| SYSROOT TAR | /usr/include (newlib, m5f, gcc), /usr/lib (libc, libm, libsoftfp, libgcc, cmsrt.o, entry31.o, image.ld) | cmsrt resolves absolute paths in it, read-only |
| CC EXEC | the driver: `CC fn` -> fn MODULE | cc1 -> as -> ld -> elf2cms -> LOAD -> GENMOD |

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

## Open

- `ld @file` response files (cmsrt's read of the response file).
- C++: libstdc++ headers in the sysroot, and a test.
- COBOL natively: cobc -> CC with libcob.a and libgmp.a in the sysroot (9.19).
- Speed: about 34 s of CPU for hello (cc1 about 25 s).
