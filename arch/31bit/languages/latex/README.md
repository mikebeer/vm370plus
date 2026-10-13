# LaTeX for VM/370plus (milestone 9.27)

This is TeX Live 2023 in the 31-bit Debian guest:
- pdfTeX 1.40.25, run as `latex` (DVI output) and `pdflatex` (PDF output);
- dvips, kpsewhich and makeindex;
- a small texmf tree (24 MB) with LaTeX 2023 and these packages: amsmath,
  amscls, graphics, tools, geometry, url and babel.

It is not a full TeX Live. Fonts are Computer Modern (Type 1, from
amsfonts) and the only hyphenation patterns are US English.

On CMS:

    LATEX fn [ft [fm]]          ft default TEX: latex twice, then dvips
                                -> fn PS A          (PSPRINT fn prints it)
    LATEX fn (PDF               pdflatex -> /var/spool/vmspool/out/<userid>/fn.pdf

## How it is built

`mktex.sh` cross-builds the programs with wcc and wc++ (`languages/common`):
- **wc++** is GCC 13's g++ for `-m31` against wheezy's glibc 2.13.
  libstdc++ is linked statically.
- `wheezy-cxx.h` and `wheezy-cxx-shim.c` supply what GCC 13's libstdc++
  expects of a newer glibc: `aligned_alloc`, `timespec_get`, the C23
  `strto*` names, `arc4random`, and `_dl_find_object` (built on
  `dl_iterate_phdr`, so C++ exceptions unwind). They also declare the
  obsolete `isinf`/`isnan`.

**The formats are made on the PC.** `latex.fmt` and `pdflatex.fmt` are
portable, and the PC's pdfTeX is the same version. Made in the guest, a
format takes more than half an hour: TeX runs about a thousand times slower
there.

**Speed in the guest.** A one-page article costs about 40 seconds per run,
and LATEX runs it twice.

## Limits

- Source lines must be at most 80 characters. The spool bridge carries
  80-column cards, so a longer line is cut.
- No fonts are made on demand: METAFONT is not built and mktexpk is off.
- No latexmk, no BibTeX yet.

## Verified (pd19, 13 October)

- `LATEX HELLO` produced 1 page with a contents list, an equation, a list
  and a table, returned as HELLO PS A.
- `PSPRINT HELLO` then `tools/psprint.py --pdf` produced the PDF on the PC.
