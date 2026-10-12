#!/bin/sh
# mkdebtests.sh -- the C89 languages for the Debian 7 guest, with their tests:
# APL, COBOL (mikebeer/cobol), PL/I (mikebeer/pli), xBase C, Smalltalk C.
# Cross-compiled by wcc (GCC 13 -m31, wheezy glibc).  Writes OUT/langs2.tgz;
# on the guest: tar xzf langs2.tgz -C / ; sh /usr/local/share/vmtests/runall.sh
# usage: mkdebtests.sh [OUT]   (sources: $MB/{apl,cobol,pli,xbase,smalltalk})
set -e
H=$(cd "$(dirname "$0")" && pwd)
MB=${MB:-/home/claude/mikebeer}
XB=${XB:-/home/claude/xbase}
OUT=${1:-/home/claude/src/deb}
R=$OUT/root; rm -rf "$R"
B=$R/usr/local/bin; T=$R/usr/local/share/vmtests
mkdir -p $B $T/apl $T/cobol $T/pli/src $T/xbase $T/smalltalk $R/usr/local/share/cobol $R/usr/local/include
W="$H/wcc -O2"
$W -o $B/apl   $MB/apl/apl_all.c -lm
$W -o $B/cobol $MB/cobol/cobol_all.c -lm
$W -o $B/pli   $MB/pli/pli_all.c -lm
$W -o $B/xbase-c $XB/c/*.c -lm
$W -std=c89 -DST_POSIX -o $B/stc $MB/smalltalk/c/st_all.c -lm
printf '%s\n' '#!/bin/sh' '# plic fn.pli [gcc options] -- PL/I (mikebeer/pli) to an executable' \
  'f=$1; shift; b=${f%.pli}' 'pli $f -o $b.c && gcc -O1 -o $b $b.c -lm "$@"' > $B/plic; chmod +x $B/plic
cp $MB/cobol/src/cobrt.h $R/usr/local/share/cobol/
cp $MB/pli/src/plirt.h $R/usr/local/include/; cp $MB/pli/src/plirt.h $T/pli/src/
cp -r $MB/apl/tests $MB/apl/examples $MB/apl/ref $T/apl/
cp -r $MB/cobol/tests $T/cobol/; cp -r $MB/pli/tests $T/pli/
cp -r $XB/tests $T/xbase/; cp -r $MB/smalltalk/lib $MB/smalltalk/tests $T/smalltalk/
cp $H/runall.sh $T/
# tests/run.sh calls $top/$COBOL (t15's dynamic module): a relative name
ln -s /usr/local/bin/cobol $T/cobol/cobol; ln -s /usr/local/bin/pli $T/pli/pli
(cd $R && tar czf "$OUT/langs2.tgz" usr)
ls -l "$OUT/langs2.tgz"
