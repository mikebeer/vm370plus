#!/bin/bash
# mklang.sh IMAGE -- the VM/370plus language disk (9.24): a 3380 EDF
# volume VMLANG (VOL1 on cylinder 0; the EDF minidisk MAINT 19F is cylinders
# 1-884), 4K blocks, with every CMS language of 9.15-9.21.  The TEXT
# decks become MODULEs on CMS afterwards: MAINT, ACCESS 19F L, LANGGEN L.
# Run with Hercules down.  Inputs are the build outputs of the language
# scripts (languages/*/mk*.sh, toolchain/mktoolchain.sh).
set -e
IMG=${1:?usage: mklang.sh IMAGE.cckd}
R=/home/claude/vm370/arch/31bit; L=$R/languages; K=$R/kit/lang
TC=${TC:-/home/claude/src/tc/cms}; NEW=${NEW:-/home/claude/src/new}
SP=${SP:-/home/claude/stanfordpascal/pascal}; PLIO=${PLIO:-/home/claude/src/lang}
LST=$(mktemp)
add() { echo "$1 $2 L1 $3 $4" >> $LST; }
# GCC 13 toolchain (9.21)
add CC1 XIMAGE $TC/cc1.ximage BIN; add CC1PLUS XIMAGE $TC/cc1plus.ximage BIN
add ELF2CMS XIMAGE $TC/elf2cms.ximage BIN
for t in as ld ar xl; do add ${t^^} TEXT $TC/$t.text BIN; done
add SYSROOT TAR $TC/sysroot.tar BIN; add CXXROOT TAR $TC/cxxroot.tar BIN
add CC EXEC $R/toolchain/cms/CC.EXEC F
# GnuCOBOL (9.19), COBX (mikebeer/cobol)
add COBC TEXT /home/claude/src/cobol/cms/cobc.text BIN; add COBOL EXEC $L/cobol/cms/COBOL.EXEC F
add COB2C TEXT $NEW/cob2c.text BIN; add COBRT H $NEW/cobrt.h V; add COBX EXEC $L/cobol2c/cms/COBX.EXEC F
# PL/I (9.20)
add PLI2C TEXT $NEW/pli2c.text BIN
for f in PLIC.EXEC PLICMS.H PLIRT.H; do add ${f%.*} ${f#*.} $PLIO/plifiles/$f F; done
# Python (9.15), SQLite + SQLSERV (9.4)
add PY XIMAGE /home/claude/src/py/py.ximage BIN; add PYTHON EXEC $L/python/cms/PYTHON.EXEC F
add SQLITE3 TEXT /home/claude/src/sqlite/out/sqlite3.text BIN; add SQLRUN TEXT /home/claude/src/sqlite/out/sqlrun.text BIN
add SQLITE EXEC $L/sqlite/cms/SQLITE.EXEC F; add SQL EXEC $L/sqlite/sqlserv/SQL.EXEC F
add SQLSERV EXEC $L/sqlite/sqlserv/SQLSERV.EXEC F
# Stanford Pascal (9.13a)
add PCINT TEXT /home/claude/src/spas/cms/pcint.text BIN
add PCPAS EXEC $L/pascal/cms/PCPAS.EXEC F; add PCRUN EXEC $L/pascal/cms/PCRUN.EXEC F
add PASCAL1 PCODE $SP/src/pascal1.pcode V; add PASCAL1 PCODE1 $SP/src/pascal1.pcode1 V
for p in paslibx passcan pasutils; do add ${p^^} PCODE $SP/src/$p.pcode V; done
add PASCAL MESSAGES $SP/etc/pascal.messages V
# APL (9.8), xBase (9.6), Smalltalk; cREXX VM for the RXBIN versions
add APLM TEXT $PLIO/apl.text BIN; add APL EXEC $L/apl/cms/APL.EXEC F
add XBASEC XIMAGE $NEW/xbasec.ximage BIN; add XBASE RXBIN /home/claude/src/xb/xbasex.rxbin RAW
add XBASE EXEC $L/xbase/cms/XBASE.EXEC F; add RXBVM8 TEXT $R/m8/rxbvm.text BIN
add SMALLTC TEXT $NEW/stc.text BIN; add SMALLTLK EXEC $L/smalltalk/cms/SMALLTLK.EXEC F
for f in $L/smalltalk/lib/*.st; do b=$(basename $f .st); add $(echo ${b:0:8} | tr a-z A-Z) ST $f V; done
# the disk's own files
add LANG EXEC $K/LANG.EXEC F; add LANGGEN EXEC $K/LANGGEN.EXEC F; add LANG README $K/README.txt V
sed -i 's/ F$/ F 80/' $LST
for f in $(awk '{print $4}' $LST); do test -f $f || { echo "### missing $f"; exit 1; }; done
rm -f "$IMG"; dasdinit -z "$IMG" 3380 VMLANG >/dev/null
python3 $R/tools/edf.py --cckd --cyl=1 "$IMG" format VMLANG 4096 | tail -1
python3 $R/tools/edf.py --cckd --cyl=1 "$IMG" putlist $LST | tail -1
cp $LST "${IMG%.cckd}.lst"; rm -f $LST
