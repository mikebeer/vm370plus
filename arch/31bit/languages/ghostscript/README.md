# Ghostscript for the Debian guest (milestone 9.29)

This is GPL Ghostscript 10.02.1, cross-built for 31-bit s390 Linux.
`mkgs.sh` builds it with wcc (GCC 13 `-m31`, wheezy's glibc) from Ubuntu's
sources: Ghostscript itself plus zlib, libjpeg9, libpng1.6, freetype,
jbig2dec and openjpeg2.

The fonts and the PostScript resources are compiled into the binary, so
`/usr/local/bin/gs` (24 MB) is all that has to be installed. These scripts
go in `/usr/local/bin` beside it:
- `ps2pdf`
- `ps2pdfwr`
- `pdf2ps`
- `ps2ps`

**Build notes**
- `arch_s390_31.h` is `base/genarch.c`, built with wcc and run in the
  guest, with `ARCH_SIZEOF_GX_COLOR_INDEX` set to 8. genarch did not see
  Ghostscript's `GX_COLOR_INDEX_TYPE`, so it reported 4.
- The OpenJPEG config headers are normally made by CMake; `mkgs.sh` writes
  them.
- `ttsubpix.c` is removed from `base/freetype.mak`, because freetype 2.13
  no longer has it.

**Verified (pd19, 13 October)** in the guest:
- `gs -v`
- groff's PostScript to PDF in 17 s, rendered back to PNG.
- `PSPRINT HELLO (BRIDGE` from CMS: the bridge's `print` service turned the
  PostScript into a 12,491-byte PDF and sent it to port 9100 on the PC.
