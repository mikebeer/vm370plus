#!/bin/sh
# mksqlite-cms.sh WORK OUT -- SQLite for CMS (VM/370plus 9.4): the sqlite3
# shell and library from the 3.46.1 amalgamation (built from Ubuntu's
# sqlite3_3.46.1.orig.tar.xz with tclsh), cross-compiled with m5f/cmscc.
#   OUT/sqlite3.text   SQLITE3 TEXT: LOAD SQLITE3 (CLEAR, GENMOD SQLITE3
# Unix VFS on cmsrt's POSIX layer: no threads, no WAL, no mmap, temp
# store in memory (temporary files would need CMS names), locks always
# granted (one user per disk), fsync writes the CMS file.
set -e
H=$(cd "$(dirname "$0")" && pwd); M=$(cd "$H/../../m5f" && pwd)
W=$(cd "$1" && pwd); O=$(mkdir -p "$2" && cd "$2" && pwd)
V=3.46.1
cd $W
[ -f sqlite3_$V.orig.tar.xz ] || curl -s -O http://archive.ubuntu.com/ubuntu/pool/main/s/sqlite3/sqlite3_$V.orig.tar.xz
[ -d sqlite3-$V ] || tar xf sqlite3_$V.orig.tar.xz
mkdir -p bld; cd bld
[ -f sqlite3.c ] || { ../sqlite3-$V/configure >/dev/null; make sqlite3.c shell.c >/dev/null; }
F="-O2 -include $H/sqlcms.h -DSQLITE_THREADSAFE=0 -DSQLITE_OMIT_LOAD_EXTENSION \
 -DSQLITE_OMIT_WAL -DSQLITE_MAX_MMAP_SIZE=0 -DHAVE_USLEEP=0 -DSQLITE_DISABLE_LFS \
 -DSQLITE_TEMP_STORE=3"
$M/cmscc $F -c sqlite3.c -o sqlite3.o
$M/cmscc $F -c shell.c -o shell.o
$M/cmscc -o sqlite3.elf shell.o sqlite3.o
${E2C:-/home/claude/m5f/elf_to_cms} sqlite3.elf $O/sqlite3.text
# SQLRUN (the SQLSERV worker): sqlserv/sqlrun.c with the same sqlite3.o
$M/cmscc $F -I. -c $H/sqlserv/sqlrun.c -o sqlrun.o
$M/cmscc -o sqlrun.elf sqlrun.o sqlite3.o
${E2C:-/home/claude/m5f/elf_to_cms} sqlrun.elf $O/sqlrun.text
