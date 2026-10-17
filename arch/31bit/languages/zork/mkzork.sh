#!/bin/sh
# mkzork.sh -- VM/370plus 9.13e: Dungeon (Zork), the public-domain C
# translation of the mainframe game (github.com/devshane/zork), for CMS.
# Fetches a fixed commit, builds it with the M5f cross compiler (cmscc,
# AMODE 31, newlib) and writes zork.text (ZORK TEXT for the language disk;
# LANGGEN makes ZORK MODULE) and dtextc.dat (DTEXTC DAT, binary).
# CMS names: the game opens DTEXTC.DAT(B) on any accessed disk and saves
# to DSAVE DAT A (binary).
set -e
H=$(cd $(dirname $0) && pwd); M=$H/../../m5f
REPO=https://github.com/devshane/zork.git; COMMIT=bcf3403
W=$(mktemp -d); trap 'rm -rf $W' EXIT
git clone -q $REPO $W/src; (cd $W/src && git checkout -q $COMMIT)
cd $W/src
sed -i 's/"dsave.dat"/"DSAVE.DAT(B)"/' dverb2.c
for f in *.c; do
  $M/cmscc -O2 -DALLOW_GDT -DTEXTFILE='"DTEXTC.DAT(B)"' -DLOCALTEXTFILE='"DTEXTC.DAT(B)"' -c $f -o ${f%.c}.o
done
$M/cmscc -O2 -o zork.elf *.o
${ELF_TO_CMS:-/home/claude/m5f/elf_to_cms} zork.elf $H/zork.text
cp dtextc.dat $H/dtextc.dat
echo "mkzork: $H/zork.text $H/dtextc.dat"
