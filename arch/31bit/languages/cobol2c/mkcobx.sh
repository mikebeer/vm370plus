#!/bin/sh
# mkcobx.sh -- mikebeer/cobol for CMS: OUT/cob2c.text (the translator, for
# LOAD and GENMOD as COB2C MODULE) and OUT/cobrt.h (the run-time header,
# with cobrt-cms.patch: record files binary on CMS) -> COBRT H.
# usage: mkcobx.sh [COBOL-SRC] [OUT]
set -e
H=$(cd "$(dirname "$0")" && pwd)
M=$H/../../m5f
SRC=${1:-/home/claude/mikebeer/cobol}
OUT=${2:-/home/claude/src/new}
E2C=${E2C:-/home/claude/m5f/elf_to_cms}
mkdir -p "$OUT"
"$M/cmscc" -O2 -o "$OUT/cob.elf" "$SRC/cobol_all.c" -lm
"$E2C" "$OUT/cob.elf" "$OUT/cob2c.text"
cp "$SRC/src/cobrt.h" "$OUT/cobrt.h"
patch -s "$OUT/cobrt.h" < "$H/cobrt-cms.patch"
ls -l "$OUT/cob2c.text" "$OUT/cobrt.h"
