#!/bin/sh
# mkcobol-cms.sh WORK -- GnuCOBOL 3.1.2 for CMS through the M5f cross route
# (GCC 13 -m31, newlib, cmsrt), step 1 of 9.19-CMS:
#   WORK/lxc/lib: libgmp.a, libcob.a, libdl.a (dlopen stubs), libcobcms.a
#   WORK/bx/cobc/cobc: a PC cobc that generates C for the target (31-bit,
#                      big-endian)
# Then:  cobccms prog.cob [more.cob]  ->  prog.text (CMS TEXT deck)
# Needs in WORK: gnucobol-3.1.2/, gmp-6.3.0+dfsg/ (sources, GMP with a doc/
# stub), and the PC build of GnuCOBOL in WORK/bpc (configure + make).
set -e
H=$(cd "$(dirname "$0")" && pwd); W=$(cd "$1" && pwd)
M=$H/../../m5f; CC=$M/cmscc; P=$W/lxc; X=s390x-linux-gnu
# GMP
rm -rf $W/gmpc $P; mkdir $W/gmpc; cd $W/gmpc
../gmp-6.3.0+dfsg/configure --host=s390-ibm-linux --build=x86_64-linux-gnu --prefix=$P \
  --disable-shared --disable-assembly ABI=32 CC=$CC CFLAGS=-O2 >/dev/null
make -j8 >/dev/null; make install >/dev/null
# CMS shims: dlopen stubs (no dynamic loading) and the POSIX names libcob uses
cp $H/cms/inc/dlfcn.h $P/include/
$CC -O2 -I$H/cms/inc -c $H/cms/cmsdl.c -o $W/cmsdl.o; $X-ar rcs $P/lib/libdl.a $W/cmsdl.o
$CC -O2 -c $H/cms/cobcms.c -o $W/cobcms.o; $X-ar rcs $P/lib/libcobcms.a $W/cobcms.o
# libcob
rm -rf $W/bc; mkdir $W/bc; cd $W/bc
../gnucobol-3.1.2/configure --host=s390-ibm-linux --build=x86_64-linux-gnu --prefix=/usr/local \
  CC=$CC CFLAGS=-O2 COB_CC=gcc GMP_CFLAGS="-I$P/include" GMP_LIBS="$P/lib/libgmp.a" \
  CPPFLAGS="-I$P/include" LDFLAGS="-L$P/lib" --disable-shared --enable-static --without-db \
  --with-curses=no --without-xml2 --with-json=no --disable-nls >/dev/null
# cross-configure writes these as `#define X $X` / `0`; the sources use #ifdef
sed -i -e 's|^#define COB_LI_IS_LL .*|/* #undef COB_LI_IS_LL */|' \
  -e 's|^#define COB_64_BIT_POINTER .*|/* #undef COB_64_BIT_POINTER */|' \
  -e 's|^#define COB_32_BIT_LONG .*|#define COB_32_BIT_LONG 1|' config.h
make -j8 -C libcob >/dev/null 2>&1
cp libcob/.libs/libcob.a $P/lib/
# the PC cobc for the target: cobc decides byte order and long size from its
# own config.h, so a PC cobc would generate little-endian binary code
rm -rf $W/bx; cp -r $W/bpc $W/bx; cd $W/bx
sed -i -e 's|^#define COB_64_BIT_POINTER .*|/* #undef COB_64_BIT_POINTER */|' \
  -e 's|^#define COB_LI_IS_LL .*|/* #undef COB_LI_IS_LL */|' config.h
printf '#ifndef WORDS_BIGENDIAN\n#define WORDS_BIGENDIAN 1\n#endif\n#define COB_32_BIT_LONG 1\n' >> config.h
make -C cobc clean >/dev/null; make -j8 -C cobc >/dev/null 2>&1
echo "mkcobol-cms: $P/lib and $W/bx/cobc/cobc"
