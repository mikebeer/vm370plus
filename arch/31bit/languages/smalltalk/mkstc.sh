#!/bin/sh
# mkstc.sh -- build the C89 Smalltalk interpreter (c/st_all.c, mikebeer/smalltalk)
#   for CMS:    OUT/stc.text  (SMALLTC TEXT; GENMOD it to SMALLTC MODULE,
#               which SMALLTLK EXEC prefers to the cREXX SMALLTLK RXBIN)
#   for Debian: OUT/stc       (31-bit, wheezy glibc, via ../common/wcc)
# A long is 32 bits on both, so SmallIntegers are +-2^30 there.
# usage: mkstc.sh [OUT]
set -e
H=$(cd "$(dirname "$0")" && pwd)
M=$H/../../m5f
OUT=${1:-/home/claude/src/new}
E2C=${E2C:-/home/claude/m5f/elf_to_cms}
mkdir -p "$OUT"
"$M/cmscc" -O2 -o "$OUT/stc.elf" "$H/c/st_all.c" -lm
"$E2C" "$OUT/stc.elf" "$OUT/stc.text"
"$H/../common/wcc" -std=c89 -O2 -DST_POSIX -o "$OUT/stc" "$H/c/st_all.c" -lm
ls -l "$OUT/stc.text" "$OUT/stc"
