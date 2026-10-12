#!/bin/sh
# xlcc.sh OUT.ximage file.c ... -- like crt.sh, for a large program that
# runs from high storage under the XL loader (XL MODULE): cmsrt with
# -DCMS_XL, entryxl.s, linked at 0 (imagexl.ld), mkximage.py.
set -e
H=$(cd "$(dirname "$0")" && pwd)
X=s390x-linux-gnu
NEWLIB=${NEWLIB:-/home/claude/nlbuild}
NLSRC=${NLSRC:-/home/claude/nlsrc/newlib-4.4.0.20231231/newlib}
GI=$($X-gcc -m31 -print-file-name=include)
LIBGCC=$($X-gcc -m31 -print-libgcc-file-name)
OUT=$1; shift
W=$(mktemp -d)
CF="${CFLAGS_PRE} -m31 -mesa -march=z900 -msoft-float -mlong-double-64 -O2 -fno-pic -fno-pie -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-stack-protector -ffunction-sections -fdata-sections -nostdinc -isystem $GI -I$H/inc -I$NEWLIB/targ-include -I$NLSRC/libc/include"
objs=""
for c in "$@"; do
  case $c in
    *.o|*.a) objs="$objs $c";;
    *) o=$W/$(basename "$c" .c).o; $X-gcc $CF ${CFLAGS_EXTRA} -c "$c" -o "$o"; objs="$objs $o";;
  esac
done
$X-gcc $CF -DCMS_XL -c "$H/cmsrt.c" -o $W/cmsrt.o
$X-as -m31 -mesa "$H/entryxl.s" -o $W/entryxl.o
$X-ld -m elf_s390 -static --emit-relocs --gc-sections -T "$H/imagexl.ld" \
   -Map "${OUT%.*}.map" -o "${OUT%.*}.elf" $objs $W/cmsrt.o $W/entryxl.o \
   --start-group ${LIBS} $NEWLIB/libc.a $NEWLIB/libm.a "$H/softfp/libsoftfp.a" $LIBGCC --end-group
python3 "$H/mkximage.py" "${OUT%.*}.elf" "$OUT"
rm -rf $W
