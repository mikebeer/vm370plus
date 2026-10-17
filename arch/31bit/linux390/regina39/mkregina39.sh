#!/bin/sh
# mkregina39.sh -- Regina REXX 3.9.5 for the 31-bit Debian guest (9.36/9.38:
# MCproject and MChelpdesk need Regina 3.9; wheezy has 3.6).  Cross-built
# with wcc from Ubuntu's source package; installed beside wheezy's Regina
# under /usr/local/regina39 (bin/regina is the static interpreter, so it
# never loads wheezy's libregina.so.3).
#   sh mkregina39.sh [workdir]  ->  workdir/regina39-s390.tgz
# The message files (*.mtb) are binary and big-endian: msgcmp is built for
# s390x and run under qemu-s390x (same layout: four unsigned ints).
set -e
W=${1:-/home/claude/src/regina39}; HERE=$(cd $(dirname $0) && pwd)
WCC=$HERE/../../languages/common/wcc
mkdir -p $W && cd $W
ls regina-rexx_3.9.5*.orig.tar.xz >/dev/null 2>&1 || apt-get source --download-only regina-rexx
[ -d regina-rexx-3.9.5 ] || tar xJf regina-rexx_3.9.5*.orig.tar.xz
cd regina-rexx-3.9.5
[ -f Makefile ] || CC=$WCC LIBS=-lrt ./configure --host=s390-linux-gnu --build=x86_64-linux-gnu \
  --prefix=/usr/local/regina39 >/dev/null
sed -i 's/-DREGINA_BITS=64/-DREGINA_BITS=32/
  s/^REXX_BINARY_LIBS = -lcrypt    -lm  -lpthread$/REXX_BINARY_LIBS = -lcrypt -lm -lpthread -lrt/
  s/^LIBREGINA_BINARY_LIBS = -lcrypt -lpthread -ldl$/LIBREGINA_BINARY_LIBS = -lcrypt -lpthread -ldl -lrt/' Makefile
make -j2 >/dev/null
s390x-linux-gnu-gcc -static -O2 -I. -DHAVE_CONFIG_H msgcmp.c -o msgcmp.s390x
qemu-s390x ./msgcmp.s390x ./en.mts ./pt.mts ./no.mts ./de.mts ./es.mts ./pl.mts ./tr.mts ./sv.mts
touch *.mtb
rm -rf $W/stage; STRIPPROG=s390x-linux-gnu-strip make install DESTDIR=$W/stage >/dev/null
cd $W/stage/usr/local/regina39
cp bin/rexx bin/regina
rm -rf share/man include share/regina-rexx/examples lib/regina-rexx/addons/librxtest* lib/regina-rexx/3.9.5/librxtest*
tar czf $W/regina39-s390.tgz -C $W/stage usr/local/regina39
ls -l $W/regina39-s390.tgz
