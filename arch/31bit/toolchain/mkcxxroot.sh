#!/bin/sh
# mkcxxroot.sh WORK OUT -- CXXROOT TAR, the C++ half of the CMS sysroot (9.21):
#   /usr/include/c++/13    the libstdc++ 13.3 headers (mklibstdcxx.sh: newlib,
#                          soft float, generic locale; without parallel,
#                          tr1, tr2, experimental, pb_ds)
#   /usr/include/c++/s390  its bits/c++config.h etc. (s390-ibm-elf)
#   /usr/lib/cxx/cxxrt.o   operator new/delete, guards, no-exception stubs
#   /usr/lib/cxx/libstdc++.a
# cmsrt reads it only for paths under /usr/include/c++ and /usr/lib/cxx.
set -e
H=$(cd "$(dirname "$0")" && pwd); M=$(cd "$H/../m5f" && pwd)
W=$(cd "$1" && pwd); O=$(mkdir -p "$2" && cd "$2" && pwd)
X=s390x-linux-gnu
LSC=${LIBSTDCXX:-$W/lscinst/usr/local}
CXXI=$LSC/include/c++/13.3.0
S=$W/cxxroot; rm -rf $S
I=$S/usr/include/c++; L=$S/usr/lib/cxx
mkdir -p $I/13 $I/s390 $L
(cd $CXXI && tar cf - --exclude=./s390-ibm-elf --exclude=./parallel \
   --exclude=./tr1 --exclude=./tr2 --exclude=./experimental \
   --exclude=./ext/pb_ds --exclude='*.gch' .) | (cd $I/13 && tar xf -)
cp -r $CXXI/s390-ibm-elf/. $I/s390/
LIBSTDCXX=$LSC $M/cmsc++ -O2 -fno-exceptions -fno-rtti -c $M/cxxrt.cc -o $L/cxxrt.o
cp $LSC/lib32/libstdc++.a $L/libstdc++.a
python3 "$H/padar.py" $L/libstdc++.a
(cd $S && tar --format=ustar -cf $O/cxxroot.tar usr)
ls -la $O/cxxroot.tar
