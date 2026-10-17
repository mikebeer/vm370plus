#!/bin/sh
# mkmariadb.sh -- MariaDB 10.11 for the 31-bit Debian 7 guest (VM/370plus
# 9.47), cross-built with wcc/wc++ (GCC 13 -m31, wheezy's glibc 2.13) from
# Ubuntu's source package.  The database of MCproject and MChelpdesk.
#   sh mkmariadb.sh [workdir]  ->  workdir/mariadb-s390.tgz (/usr/local/mariadb)
# Steps: ncurses (static, for the client's readline), the host tools of a
# native build (import_executables: comp_err, gen_lex_hash, ...), then the
# cross build.  Server only: InnoDB, Aria, MyISAM; no performance_schema,
# Galera, RocksDB, Mroonga, Spider, CONNECT, S3, PAM/GSSAPI.  The server
# links WolfSSL (bundled); the client library has no TLS (local socket).
# Not PIE (SECURITY_HARDENED=OFF): smaller and faster code on 31-bit.  The
# first build died at its first C++ global (decimal_zero): wc++ linked the
# cross toolchain's glibc 2.39 crt1.o, which leaves constructors to libc;
# wcc/wc++ now link wheezy's own start files.
# Needs about 2.5 GB of disk.  Two hours on two CPUs.
set -e
W=${1:-/home/claude/src/mariadb}; HERE=$(cd $(dirname $0) && pwd)
CM=$HERE/../../languages/common
V=10.11.14
mkdir -p $W && cd $W
ls mariadb_$V.orig.tar.gz >/dev/null 2>&1 || apt-get source --download-only mariadb
S=$W/mariadb-$V
if [ ! -d $S ]; then
  # the test suites and the engines we do not build stay in the tarball
  tar xzf mariadb_$V.orig.tar.gz --exclude='*/mysql-test/suite' --exclude='*/mysql-test/main' \
    --exclude='*/mysql-test/plugin' --exclude='*/storage/rocksdb' --exclude='*/storage/mroonga' \
    --exclude='*/storage/spider' --exclude='*/storage/columnstore' --exclude='*/storage/connect' \
    --exclude='*/storage/tokudb'
  (cd $W && patch -p1 < $HERE/no-htm.patch)
fi
# 1. ncurses 6.4, static, into $W/tgt
T=$W/tgt
if [ ! -f $T/usr/lib/libncurses.a ]; then
  mkdir -p $W/nc && cd $W/nc
  ls ncurses_*.orig.tar.gz >/dev/null 2>&1 || apt-get source --download-only ncurses
  tar xzf ncurses_*.orig.tar.gz && cd ncurses-6*
  CC=$CM/wcc ./configure --host=s390-linux-gnu --build=x86_64-linux-gnu --prefix=/usr \
    --without-cxx --without-cxx-binding --without-ada --without-progs --without-tests \
    --without-manpages --without-shared --with-normal --disable-widec --disable-stripping \
    --with-default-terminfo-dir=/usr/share/terminfo \
    --with-terminfo-dirs=/etc/terminfo:/lib/terminfo:/usr/share/terminfo >/dev/null
  make -j2 >/dev/null && make install DESTDIR=$T >/dev/null
fi
# 2. host tools
mkdir -p $W/native && cd $W/native
[ -f import_executables.cmake ] || {
  cmake $S -G Ninja -DCMAKE_BUILD_TYPE=Release -DPLUGIN_ROCKSDB=NO -DPLUGIN_MROONGA=NO \
    -DPLUGIN_SPIDER=NO -DPLUGIN_CONNECT=NO -DPLUGIN_COLUMNSTORE=NO -DWITH_UNIT_TESTS=OFF \
    -DWITH_WSREP=OFF -DWITH_EMBEDDED_SERVER=OFF >/dev/null
  ninja -j2 import_executables; }
# 3. the cross build; CPATH puts ncurses' curses.h/term.h in reach of the
# configure checks and of the client
export MDB_TGT=$T CPATH=$T/usr/include
mkdir -p $W/cross && cd $W/cross
[ -f build.ninja ] || cmake $S -G Ninja -DCMAKE_TOOLCHAIN_FILE=$HERE/s390-wheezy.cmake \
  -DIMPORT_EXECUTABLES=$W/native/import_executables.cmake \
  -DCMAKE_BUILD_TYPE=MinSizeRel -DCMAKE_INSTALL_PREFIX=/usr/local/mariadb \
  -DMYSQL_DATADIR=/var/lib/mariadb -DMYSQL_UNIX_ADDR=/var/run/mysqld/mysqld.sock \
  -DWITH_SSL=bundled -DCONC_WITH_SSL=NONE -DWITH_ZLIB=bundled -DWITH_PCRE=bundled \
  -DWITH_LIBFMT=bundled -DWITH_WSREP=OFF -DWITH_EMBEDDED_SERVER=OFF -DWITH_UNIT_TESTS=OFF \
  -DWITH_SAFEMALLOC=OFF -DWITH_MARIABACKUP=OFF -DPLUGIN_ROCKSDB=NO -DPLUGIN_MROONGA=NO \
  -DPLUGIN_SPIDER=NO -DPLUGIN_CONNECT=NO -DPLUGIN_COLUMNSTORE=NO -DPLUGIN_S3=NO \
  -DPLUGIN_OQGRAPH=NO -DPLUGIN_SPHINX=NO -DPLUGIN_AUTH_PAM=NO -DPLUGIN_AUTH_PAM_V1=NO \
  -DPLUGIN_AUTH_GSSAPI=NO -DPLUGIN_AUTH_GSSAPI_CLIENT=NO -DPLUGIN_FEDERATEDX=NO \
  -DPLUGIN_PERFSCHEMA=NO -DPLUGIN_HASHICORP_KEY_MANAGEMENT=NO -DPLUGIN_PROVIDER_LZ4=NO \
  -DPLUGIN_PROVIDER_LZO=NO -DPLUGIN_PROVIDER_SNAPPY=NO -DPLUGIN_PROVIDER_BZIP2=NO \
  -DPLUGIN_PROVIDER_LZMA=NO -DPLUGIN_TYPE_MYSQL_JSON=NO \
  -DCURSES_LIBRARY=$T/usr/lib/libncurses.a -DCURSES_INCLUDE_PATH=$T/usr/include \
  -DSTACK_DIRECTION=-1 -DSECURITY_HARDENED=OFF
ninja -j2
rm -rf $W/stage; DESTDIR=$W/stage ninja install >/dev/null
cd $W/stage/usr/local/mariadb
# the Debian disk has room, but nothing here is used on it
rm -rf mysql-test sql-bench man include lib/*.a lib/pkgconfig
rm -f bin/mariadb-client-test bin/mysql_client_test bin/mariadb-test bin/mysqltest \
  bin/mariadb-slap bin/mysqlslap
for f in bin/* lib/*.so* lib/plugin/*.so; do
  [ -L $f ] || ! file $f | grep -q ELF || s390x-linux-gnu-strip $f; done
tar czf $W/mariadb-s390.tgz -C $W/stage usr/local/mariadb
ls -l $W/mariadb-s390.tgz
