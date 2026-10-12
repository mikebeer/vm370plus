#!/bin/sh
# mktoolchain.sh WORK OUT -- the C/C++ toolchain for CMS (VM/370plus 9.21):
# binutils 2.42 (as, ld, ar) and GCC 13.3 (cc1, cc1plus), cross-built with
# m5f/cmscc and m5f/cmsc++ for the M5f runtime (newlib + cmsrt), plus the
# sysroot tar the compilers read their headers and libraries from.
#   OUT/as.text ld.text ar.text     TEXT decks: LOAD, GENMOD (on an
#                                   800-byte disk: I-271)
#   OUT/cc1.ximage cc1plus.ximage   run from high storage: XL CC1 ...
#   OUT/elf2cms.ximage              elf_to_cms (TEXT decks) on CMS
#   OUT/xl.text                     the XL loader
#   OUT/sysroot.tar                 SYSROOT TAR: /usr/include, /usr/lib
#   OUT/cxxroot.tar                 CXXROOT TAR: libstdc++ headers, library
# WORK needs: binutils-2.42/, gcc-13.3.0/ (Ubuntu binutils-source and
# gcc-13-source), mpfr-4.2.1/, mpc-1.3.1/ (archive.ubuntu.com pool), the
# CMS GMP from languages/cobol/mkcobol-cms.sh in $GMPCMS, elf_to_cms.c with
# m5f/elf_to_cms-pcrel.patch and m5f/elf_to_cms-cxx.patch applied.
set -e
H=$(cd "$(dirname "$0")" && pwd); M=$(cd "$H/../m5f" && pwd)
W=$(cd "$1" && pwd); O=$(mkdir -p "$2" && cd "$2" && pwd)
X=s390x-linux-gnu
GMPCMS=${GMPCMS:-/home/claude/src/cobol/lxc}
E2C=${E2C:-/home/claude/m5f/elf_to_cms.c}
TOOLS="AR=$X-ar RANLIB=$X-ranlib NM=$X-nm OBJDUMP=$X-objdump STRIP=$X-strip AS=$X-as LD=$X-ld"
gcc -O2 -o $W/e2c $E2C                       # elf_to_cms for the PC

# --- libstdc++ for newlib and soft float: cmsc++ links C++ with it ---------
sh "$H/mklibstdcxx.sh" $W
export LIBSTDCXX=$W/lscinst/usr/local

# --- binutils: host = CMS, target = s390 31-bit ELF ------------------------
rm -rf $W/bb; mkdir $W/bb; cd $W/bb
env ac_cv_tls=none $W/binutils-2.42/configure --host=s390-ibm-elf --build=x86_64-linux-gnu \
  --target=s390-linux-gnu --prefix=/cms CC=$M/cmscc CFLAGS=-O2 CC_FOR_BUILD=gcc $TOOLS \
  --disable-nls --disable-werror --disable-gdb --disable-gdbserver --disable-sim \
  --disable-gprof --disable-gprofng --disable-plugins --disable-shared --enable-static \
  --without-zstd --without-debuginfod --disable-libctf --disable-readline \
  --without-system-zlib >/dev/null
make -j8 MAKEINFO=true configure-libiberty >/dev/null
# libiberty's lrealpath() returns nothing without realpath(): its callers
# free() the argument (cmsrt provides realpath)
sed -i 's|/\* #undef HAVE_REALPATH \*/|#define HAVE_REALPATH 1|' libiberty/config.h
make -j8 MAKEINFO=true all-gas all-ld all-binutils >/dev/null
for p in "as gas/as-new" "ld ld/ld-new" "ar binutils/ar"; do
  set -- $p; $W/e2c $2 $O/$1.text; done

# --- MPFR, MPC for the CMS host ---------------------------------------------
for lib in mpfr-4.2.1 mpc-1.3.1; do
  rm -rf $W/b-$lib; mkdir $W/b-$lib; cd $W/b-$lib
  $W/$lib/configure --host=s390-ibm-elf --build=x86_64-linux-gnu --prefix=$W/host \
    CC=$M/cmscc CFLAGS=-O2 $TOOLS --with-gmp=$GMPCMS --with-mpfr=$W/host \
    --disable-shared --disable-thread-safe --disable-decimal-float --disable-float128 >/dev/null
  make -j8 >/dev/null; make install >/dev/null
done

# --- GCC 13: cc1, cc1plus (Canadian cross: build PC, host CMS) --------------
rm -rf $W/bg; mkdir $W/bg; cd $W/bg
env ac_cv_tls=none $W/gcc-13.3.0/configure --build=x86_64-linux-gnu --host=s390-ibm-elf \
  --target=s390-linux-gnu --prefix=/cms CC=$M/cmscc CXX=$M/cmsc++ CFLAGS=-O2 \
  CXXFLAGS="-O2 -DCODY_NETWORKING=0" $TOOLS CC_FOR_BUILD=gcc CXX_FOR_BUILD=g++ \
  AR_FOR_TARGET=$X-ar AS_FOR_TARGET=$X-as LD_FOR_TARGET=$X-ld NM_FOR_TARGET=$X-nm \
  RANLIB_FOR_TARGET=$X-ranlib GCC_FOR_TARGET="$X-gcc -m31" CC_FOR_TARGET="$X-gcc -m31" \
  CXX_FOR_TARGET="$X-g++ -m31" --enable-languages=c,c++ --disable-bootstrap --disable-nls \
  --disable-shared --disable-multilib --disable-plugin --disable-lto --without-zstd \
  --without-isl --disable-libssp --disable-libquadmath --disable-libgomp --disable-libatomic \
  --disable-threads --disable-decimal-float --with-gmp-include=$GMPCMS/include \
  --with-gmp-lib=$GMPCMS/lib --with-mpfr=$W/host --with-mpc=$W/host --with-arch=z900 \
  --with-mode=esa --with-long-double-128=no --with-system-zlib=no >/dev/null
make -j16 MAKEINFO=true configure-libiberty >/dev/null
sed -i 's|/\* #undef HAVE_REALPATH \*/|#define HAVE_REALPATH 1|' libiberty/config.h
make -j16 MAKEINFO=true all-gcc >/dev/null 2>&1 || true   # host tools we do not need may fail
cd gcc; rm -f cc1 cc1plus
CMSCC_XL=1 make cc1 cc1plus MAKEINFO=true >/dev/null
for p in cc1 cc1plus; do python3 $M/mkximage.py $p $O/$p.ximage; done

# --- elf_to_cms and the XL loader for CMS -----------------------------------
CMSCC_XL=1 $M/cmscc -O2 -w -o $W/elf2cms.xelf $E2C
python3 $M/mkximage.py $W/elf2cms.xelf $O/elf2cms.ximage
CFLAGS_PRE="-I$M/inc" sh $M/crt.sh $O/xl.text $M/xl.c

# --- the sysroot -------------------------------------------------------------
NL=${NLSRC:-/home/claude/nlsrc/newlib-4.4.0.20231231/newlib}
NLB=${NEWLIB:-/home/claude/nlbuild}
S=$W/sysroot; rm -rf $S
mkdir -p $S/usr/include/newlib $S/usr/include/m5f/sys $S/usr/lib/gcc
cp -r $NL/libc/include/. $S/usr/include/newlib/; cp -r $NLB/targ-include/. $S/usr/include/newlib/
cp $M/inc/setjmp.h $S/usr/include/m5f/; cp $M/inc/sys/dirent.h $S/usr/include/m5f/sys/
cp -r $($X-gcc -m31 -print-file-name=include) $S/usr/lib/gcc/include
cp $NLB/libc.a $NLB/libm.a $($X-gcc -m31 -print-libgcc-file-name) $S/usr/lib/
cp $M/softfp/libsoftfp.a $S/usr/lib/libsoftf.a; cp $M/image.ld $S/usr/lib/
$M/cmscc -O2 -c $M/cmsrt.c -o $S/usr/lib/cmsrt.o
$X-as -m31 -mesa $M/entry31.s -o $S/usr/lib/entry31.o
# archives travel on 80-byte cards: pad each to a multiple of 80 with a
# dummy member, which the archive index never names
python3 "$H/padar.py" $S/usr/lib/*.a
(cd $S && tar --format=ustar -cf $O/sysroot.tar usr)
sh "$H/mkcxxroot.sh" $W $O                 # CXXROOT TAR: the C++ half
echo "mktoolchain: $O"
