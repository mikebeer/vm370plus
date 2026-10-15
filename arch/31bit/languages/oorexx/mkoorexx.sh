#!/bin/bash
# mkoorexx.sh SRC OUT -- 9.9: ooRexx 5 for the Debian guest (31-bit s390
# Linux), cross-built on the PC.  SRC: github ooRexx/ooRexx.  OUT gets
# oorexx5-s390-31.tar.gz (/opt/oorexx).  Debian 7's glibc (2.13) is too old
# for it, so the package carries the lx31 glibc 2.39 (ld.so.1, libc, libm) and
# its programs use /opt/oorexx/lib/ld.so.1 as their interpreter; libstdc++ and
# libgcc are linked statically.  rexx.img cannot be made on the PC (31-bit
# s390 binaries do not run there): in the guest, once,
#   cd /opt/oorexx/bin && ./rexximage /opt/oorexx/lib/rexx.img
set -e
H=$(cd "$(dirname "$0")" && pwd)
S=$(cd "$1" && pwd); O=$(mkdir -p "$2" && cd "$2" && pwd)
# no ncurses for 31-bit s390 here: build orxncurses only when it is found
sed -i 's/^if (NOT WIN32)$/if (NOT WIN32 AND CURSES_FOUND)/' $S/CMakeLists.txt
B=$O/build; rm -rf $B; mkdir -p $B; cd $B
cmake $S -DCMAKE_TOOLCHAIN_FILE=$H/tc-s390-31.cmake -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/opt/oorexx -DORX_ENABLE_IPO=OFF >cmake.log
make -k >make.log 2>&1 || true          # only rexx.img fails (it runs rexximage)
T=$O/stage/opt/oorexx; rm -rf $O/stage; mkdir -p $T/bin $T/lib
cp -a bin/* $T/bin/; cp -a lib/*.so* $T/lib/
rm -f $T/bin/{callrexx1,callrexx2,provoke_locks,provoke_locks.rex,stackOverflow,runRexxProgram,rexxinstance}
rm -f $T/lib/libexternal_* $T/lib/liborx*.so $T/lib/librexxapi[123].so
S31=/home/claude/lx31/sysroot/lib; cp -L $S31/ld.so.1 $S31/libc.so.6 $S31/libm.so.6 $T/lib/
for b in $T/bin/*; do file $b | grep -q ELF && s390x-linux-gnu-strip $b; done
for l in $T/lib/lib{rexx,rexxapi,rxmath,rxregexp,rxsock,rxunixsys,hostemu}*.so.*; do [ -L $l ] || s390x-linux-gnu-strip --strip-unneeded $l; done
tar -C $O/stage -czf $O/oorexx5-s390-31.tar.gz opt
ls -la $O/oorexx5-s390-31.tar.gz
