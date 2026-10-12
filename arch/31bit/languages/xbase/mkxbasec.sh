#!/bin/sh
# mkxbasec.sh -- build the C89 xBase interpreter (mikebeer/xbase c/)
#   for CMS:    OUT/xbasec.ximage  (XBASEC XIMAGE, run by XBASE EXEC via XL)
#   for Debian: OUT/xbase-c        (31-bit, wheezy glibc, via ../common/wcc)
# usage: mkxbasec.sh [XBASE-SRC] [OUT]
set -e
H=$(cd "$(dirname "$0")" && pwd)
M=$H/../../m5f
SRC=${1:-/home/claude/xbase}
OUT=${2:-/home/claude/src/new}
mkdir -p "$OUT"
CMSCC_XL=1 "$M/cmscc" -O2 -o "$OUT/xbc.elf" "$SRC"/c/*.c -lm
python3 "$M/mkximage.py" "$OUT/xbc.elf" "$OUT/xbasec.ximage"
"$H/../common/wcc" -O2 -o "$OUT/xbase-c" "$SRC"/c/*.c -lm
ls -l "$OUT/xbasec.ximage" "$OUT/xbase-c"
