#!/bin/sh
# mkgs.sh -- Ghostscript 10.02.1 for the 31-bit Debian guest (VM/370plus 9.29)
# Cross-built with wcc (GCC 13 -m31, wheezy's glibc).  The Ubuntu source is
# "dfsg": the libraries Ghostscript builds from its own tree are missing and
# come from their Ubuntu source packages.  Needs deb-src in apt.
#   sh mkgs.sh [workdir]     ->  workdir/ghostscript-*/bin/gs
set -e
W=${1:-/home/claude/src/gs}; HERE=$(cd $(dirname $0) && pwd)
WCC=$HERE/../common/wcc
mkdir -p $W && cd $W
for p in ghostscript zlib libjpeg9 libpng1.6 freetype jbig2dec openjpeg2; do
  ls ${p}_*.orig.tar.* >/dev/null 2>&1 || apt-get source --download-only $p
done
for t in *.orig.tar.*; do tar xf $t; done
G=$(ls -d ghostscript-10*/ | head -1); G=${G%/}
cp -r zlib-*/ $G/zlib; cp -r jpeg-9*/ $G/jpeg; cp -r libpng-1.6*/ $G/libpng
cp -r freetype-2*/ $G/freetype; cp -r jbig2dec-*/ $G/jbig2dec
cp -r openjpeg-2*/ $G/openjpeg
cd $G
# OpenJPEG's config headers are made by CMake; these are the s390 answers
O=openjpeg/src/lib/openjp2
printf '#define OPJ_HAVE_STDINT_H 1\n#define OPJ_VERSION_MAJOR 2\n#define OPJ_VERSION_MINOR 5\n#define OPJ_VERSION_BUILD 0\n' > $O/opj_config.h
printf '#define OPJ_HAVE_INTTYPES_H 1\n#define OPJ_PACKAGE_VERSION "2.5.0"\n#define OPJ_HAVE_FSEEKO 1\n#define OPJ_HAVE_MALLOC_H 1\n#define OPJ_HAVE_MEMALIGN 1\n#define OPJ_HAVE_POSIX_MEMALIGN 1\n#if !defined(_POSIX_C_SOURCE)\n#define _POSIX_C_SOURCE 200112L\n#endif\n#define OPJ_BIG_ENDIAN\n' > $O/opj_config_private.h
# freetype 2.13 has no ttsubpix.c any more
sed -i '/ttsubpix/d' base/freetype.mak
# arch.h: base/genarch.c run in the Debian guest, GX_COLOR_INDEX 8 bytes
CC=$WCC CCAUX=gcc ./configure --host=s390-linux-gnu --build=x86_64-linux-gnu \
  --with-arch_h=$HERE/arch_s390_31.h --disable-cups --disable-gtk \
  --disable-fontconfig --disable-dbus --without-libtiff --without-libidn \
  --without-libpaper --without-tesseract --without-ijs --without-urf \
  --without-so --without-cal --without-x --disable-threading \
  --disable-hidden-visibility --prefix=/usr/local
make -j8 so=no
s390x-linux-gnu-strip -o bin/gs.strip bin/gs
ls -la bin/gs bin/gs.strip
