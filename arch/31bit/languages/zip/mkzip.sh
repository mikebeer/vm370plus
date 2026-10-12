#!/bin/sh
# mkzip.sh SRC OUT -- zip and unzip (Info-ZIP 3.0 / 6.0) and gzip (zlib
# 1.3.1's minigzip) for CMS (9.23): OUT/izzip.text, izunzip.text, minigz.text
# (LOAD, GENMOD as IZZIP, IZUNZIP, MINIGZ MODULE; ZIP, UNZIP, GZIP EXEC).
# SRC has zip30/, unzip60/, zlib-1.3.1/ (Ubuntu archive orig tarballs).
set -e
H=$(cd "$(dirname "$0")" && pwd); M=$H/../../m5f; S=$(cd "$1" && pwd); O=$(mkdir -p "$2" && cd "$2" && pwd)
E2C=${E2C:-/home/claude/m5f/elf_to_cms}
U="-DUNIX -DNO_SYMLINKS -DNO_LCHMOD -DNO_LCHOWN -DNO_CRYPT -Dtimezone=_timezone"
cd $S/unzip60
$M/cmscc -O2 $U -DUNZIP_STUBS -I. -o $O/izunzip.elf unzip.c crc32.c crypt.c envargs.c \
  explode.c extract.c fileio.c globals.c inflate.c list.c match.c process.c ubz2err.c \
  unreduce.c unshrink.c zipinfo.c unix/unix.c $H/zipcms.c
cd $S/zip30
$M/cmscc -O2 $U -DHAVE_DIRENT_H -DIZ_PWLEN=80 -DNO_BZIP2_SUPPORT -I. -Iunix -o $O/izzip.elf \
  zip.c zipfile.c zipup.c fileio.c util.c globals.c crypt.c crc32.c deflate.c trees.c \
  unix/unix.c $H/zipcms.c
cd $S/zlib-1.3.1      # gzip: fn.ft -> fn.ft-gz (filetype FT-GZ, binary in cmsrt)
$M/cmscc -O2 '-DGZ_SUFFIX="-gz"' -I. -o $O/minigz.elf test/minigzip.c adler32.c crc32.c \
  deflate.c infback.c inffast.c inflate.c inftrees.c trees.c zutil.c compress.c uncompr.c \
  gzclose.c gzlib.c gzread.c gzwrite.c
for p in izunzip izzip minigz; do $E2C $O/$p.elf $O/$p.text; done
