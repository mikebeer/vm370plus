#!/bin/sh
# mklibstdcxx.sh WORK -- libstdc++ 13.3 for CMS (9.21): built from the GCC
# source for newlib, 31-bit, soft float (the M5f ABI), generic "C" locale,
# no threads.  Ubuntu's 32-bit s390 libstdc++ will not do: it is built for
# glibc and hard float (doubles in floating-point registers), so every
# double that crosses between it and a CMS program or newlib is garbage.
# Result: WORK/lscinst/usr/local/{include/c++/13.3.0,lib32/libstdc++.a},
# used by m5f/cmsc++ (LIBSTDCXX) and mkcxxroot.sh.
# WORK needs gcc-13.3.0/ (as mktoolchain.sh).
set -e
H=$(cd "$(dirname "$0")" && pwd); M=$(cd "$H/../m5f" && pwd)
W=$(cd "$1" && pwd)
X=s390x-linux-gnu
GI=$($X-gcc -m31 -print-file-name=include)
NL=${NEWLIB:-/home/claude/nlbuild}
NS=${NLSRC:-/home/claude/nlsrc/newlib-4.4.0.20231231/newlib}
F="-m31 -mesa -march=z900 -msoft-float -mlong-double-64 -fno-pic -fno-pie \
 -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-stack-protector -U_FORTIFY_SOURCE \
 -ffunction-sections -fdata-sections -isystem $GI -I$M/inc -isystem $NL/targ-include \
 -isystem $NS/libc/include -D__CMS__ -U__unix__ -U__unix -Uunix -U__linux__ -U__linux \
 -Ulinux -U__gnu_linux__ -D_Thread_local= -D__thread= -D__SINGLE_THREAD__"
G=$W/gcc-13.3.0; [ -d $G ] || G=$W/gcc/gcc-13.3.0
B=$W/blsc; rm -rf $B; mkdir -p $B; cd $B
printf '#!/bin/sh\nexec %s-gcc -nostdinc %s "$@"\n' $X "$F" > ccw
printf '#!/bin/sh\nexec %s-g++ -nostdinc -nostdinc++ %s "$@"\n' $X "$F" > cxxw
chmod +x ccw cxxw
$G/libstdc++-v3/configure --host=s390-ibm-elf --build=x86_64-linux-gnu \
  --with-newlib --disable-shared --enable-static --disable-libstdcxx-pch --disable-nls \
  --disable-multilib --enable-clocale=generic --enable-cstdio=stdio \
  --disable-libstdcxx-threads --disable-libstdcxx-filesystem-ts \
  --disable-libstdcxx-backtrace --enable-libstdcxx-time=no \
  CC=$B/ccw CXX=$B/cxxw AR=$X-ar RANLIB=$X-ranlib AS=$X-as LD=$X-ld NM=$X-nm \
  CFLAGS=-O2 CXXFLAGS=-O2 >cfg.log 2>&1
# the compiler says "Thread model: posix"; CMS has one thread
make -j16 -C include >make.log 2>&1
cp $H/gthr-single.h include/s390-ibm-elf/bits/gthr-default.h
touch include/s390-ibm-elf/bits/gthr-default.h
make -j16 >>make.log 2>&1
rm -rf $W/lscinst; make install DESTDIR=$W/lscinst >inst.log 2>&1
ls -la $W/lscinst/usr/local/lib32/libstdc++.a
