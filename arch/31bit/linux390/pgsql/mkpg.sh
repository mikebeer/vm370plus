#!/bin/sh
# mkpg.sh -- PostgreSQL 16 for the 31-bit Debian guest (VM/370plus 9.4),
# cross-built with wcc (GCC 13 -m31, wheezy's glibc 2.13) from Ubuntu's
# source package.  No readline, zlib, ICU or OpenSSL (none in the guest's
# image of wheezy that wcc links against).
#   sh mkpg.sh [workdir]   ->  workdir/pgsql16-s390.tgz (/usr/local/pgsql)
set -e
W=${1:-/home/claude/src/pg}; HERE=$(cd $(dirname $0) && pwd)
WCC=$HERE/../../languages/common/wcc
mkdir -p $W && cd $W
ls postgresql-16_*.orig.tar.gz >/dev/null 2>&1 || apt-get source --download-only postgresql-16
S=$(tar tzf postgresql-16_*.orig.tar.gz | head -1 | cut -d/ -f1)
[ -d $S ] || tar xzf postgresql-16_*.orig.tar.gz
cd $S
CC=$WCC ./configure --host=s390-linux-gnu --build=x86_64-linux-gnu \
  pgac_cv_prog_cc_LDFLAGS_EX_BE__Wl___export_dynamic=yes \
  --prefix=/usr/local/pgsql --without-readline --without-zlib --without-icu --without-openssl
make -j8
rm -rf $W/stage; make install DESTDIR=$W/stage
cd $W/stage/usr/local/pgsql
s390x-linux-gnu-strip bin/* lib/*.so* 2>/dev/null || true
tar czf $W/pgsql16-s390.tgz -C $W/stage usr/local/pgsql
