#!/bin/sh
# mkcobol-linux.sh WORK OUT -- GnuCOBOL 3.1.2 for Debian 7 (wheezy) s390 on
# VM/370plus: cobc, cobcrun, static libcob and GMP 6.3, cross-built with
# GCC 13 -m31 against wheezy's own glibc 2.13 (so the binaries need nothing
# newer than GLIBC_2.11). Result: OUT/gnucobol-3.1.2-s390.tgz, unpacked in /
# on Debian; `cobc -x prog.cob` then uses Debian's gcc 4.6.
#
# Needs in WORK: gnucobol-3.1.2/ and gmp-6.3.0+dfsg/ (Debian source tarballs;
# the dfsg GMP has no doc/, so an empty doc/Makefile.in stub is needed),
# and the wheezy .debs libc6, libc6-dev, linux-libc-dev (DEBS=dir).
set -e
W=$(cd "$1" && pwd); O=$(mkdir -p "$2" && cd "$2" && pwd)
DEBS=${DEBS:-$W/debs}
S=$W/wsys; P=$W/lxw
# 1. wheezy sysroot; absolute .so links made relative so -ldl finds the .so
rm -rf $S; mkdir -p $S
for d in $DEBS/libc6_2.13*_s390.deb $DEBS/libc6-dev_2.13*_s390.deb $DEBS/linux-libc-dev_*_s390.deb; do
  dpkg-deb -x $d $S; done
( cd $S/usr/lib/s390-linux-gnu
  for l in $(find . -maxdepth 1 -type l); do t=$(readlink $l)
    case $t in /*) ln -sf ../../..$t $l;; esac; done )
GI=$(s390x-linux-gnu-gcc -print-file-name=include)
WCC="s390x-linux-gnu-gcc -m31 -nostdinc -isystem $GI -isystem $S/usr/include/s390-linux-gnu -isystem $S/usr/include --sysroot=$S -B$S/usr/lib/s390-linux-gnu -L$S/usr/lib/s390-linux-gnu -L$S/lib/s390-linux-gnu -Wl,-rpath-link,$S/lib/s390-linux-gnu -no-pie"
# 2. GMP (generic C, 32-bit limbs)
rm -rf $W/gmpw $P; mkdir $W/gmpw; cd $W/gmpw
../gmp-6.3.0+dfsg/configure --host=s390-linux-gnu --prefix=$P --disable-shared \
  --disable-assembly ABI=32 CC="$WCC" >/dev/null
make -j8 >/dev/null; make install >/dev/null
# 3. GnuCOBOL
rm -rf $W/bw; mkdir $W/bw; cd $W/bw
../gnucobol-3.1.2/configure --host=s390-linux-gnu --prefix=/usr/local CC="$WCC" COB_CC=gcc \
  GMP_CFLAGS="-I$P/include" GMP_LIBS="$P/lib/libgmp.a" CPPFLAGS="-I$P/include" LDFLAGS="-L$P/lib" \
  --disable-shared --enable-static --without-db --with-curses=no --without-xml2 \
  --with-json=no --disable-nls >/dev/null
# configure cannot run its size tests when cross-compiling and writes
# `#define COB_LI_IS_LL $COB_LI_IS_LL`. The sources test it with #ifdef, so
# libcob then treats 32-bit longs as 64-bit: 10-19 digit values come out wrong
# (2**60 -> 0). Also give cobc the paths it will have on Debian.
sed -i -e 's|^#define COB_LI_IS_LL .*|/* #undef COB_LI_IS_LL (31-bit) */|' \
  -e 's|^#define COB_32_BIT_LONG .*|#define COB_32_BIT_LONG 1|' \
  -e 's|^#define COB_64_BIT_POINTER .*|/* #undef COB_64_BIT_POINTER */|' \
  -e 's|^#define COB_CFLAGS .*|#define COB_CFLAGS "-pipe -I/usr/local/include -Wno-unused -fsigned-char -Wno-pointer-sign"|' \
  -e 's|^#define COB_LDFLAGS .*|#define COB_LDFLAGS ""|' \
  -e 's|^#define COB_LIBS .*|#define COB_LIBS "-L/usr/local/lib -lcob -lgmp -lm -ldl"|' config.h
L="$P/lib/libgmp.a -lm -ldl"
make -j8 LIBS="$L" >/dev/null
rm -rf $W/stage; make install DESTDIR=$W/stage LIBS="$L" >/dev/null
cp $P/lib/libgmp.a $W/stage/usr/local/lib/; cp $P/include/gmp.h $W/stage/usr/local/include/
cd $W/stage; rm -rf usr/local/share/info usr/local/share/man
s390x-linux-gnu-strip usr/local/bin/cobc usr/local/bin/cobcrun
tar czf $O/gnucobol-3.1.2-s390.tgz usr
echo "mkcobol-linux: $O/gnucobol-3.1.2-s390.tgz"
