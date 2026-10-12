#!/bin/sh
# mkcxxroot.sh WORK OUT -- CXXROOT TAR, the C++ half of the CMS sysroot (9.21):
#   /usr/include/c++/m5f   newlib os_defines.h, single-thread gthr (first)
#   /usr/include/c++/13    the libstdc++ 13 headers (no parallel, tr1,
#                          experimental, pb_ds: not for a 31-bit CMS)
#   /usr/include/c++/s390  bits/c++config.h etc. for -m31
#   /usr/lib/cxx/cxxrt.o   new/delete, __cxa_* (cxxrt.cc), and what the
#                          glibc-built libstdc++ asks of libc (cxxglibc.c)
#   /usr/lib/cxx/libstdc++.a   the 32-bit s390 libstdc++
# cmsrt reads it only for paths under /usr/include/c++ and /usr/lib/cxx.
set -e
H=$(cd "$(dirname "$0")" && pwd); M=$(cd "$H/../m5f" && pwd)
W=$(cd "$1" && pwd); O=$(mkdir -p "$2" && cd "$2" && pwd)
X=s390x-linux-gnu
CXXI=/usr/$X/include/c++/13
S=$W/cxxroot; rm -rf $S
I=$S/usr/include/c++; L=$S/usr/lib/cxx
mkdir -p $I/m5f $I/13 $I/s390 $L
cp -r $M/inc/c++/. $I/m5f/
(cd $CXXI && tar cf - --exclude=./$X --exclude=./parallel \
   --exclude=./tr1 --exclude=./tr2 --exclude=./experimental \
   --exclude=./ext/pb_ds --exclude=./profile --exclude='*.gch' .) | (cd $I/13 && tar xf -)
cp -r $CXXI/$X/32/. $I/s390/
$M/cmsc++ -O2 -fno-exceptions -fno-rtti -c $M/cxxrt.cc -o $W/cxxrt1.o
$M/cmscc -O2 -c $M/cxxglibc.c -o $W/cxxglibc.o
$X-ld -m elf_s390 -r $W/cxxrt1.o $W/cxxglibc.o -o $L/cxxrt.o
cp $($X-g++ -m31 -print-file-name=libstdc++.a) $L/libstdc++.a
python3 "$H/padar.py" $L/libstdc++.a
(cd $S && tar --format=ustar -cf $O/cxxroot.tar usr)
ls -la $O/cxxroot.tar
