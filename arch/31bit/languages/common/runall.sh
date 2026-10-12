#!/bin/sh
# runall.sh -- the language tests on Debian (VM/370plus): APL, COBOL
# (mikebeer/cobol), PL/I (mikebeer/pli), xBase and Smalltalk (C89 versions)
T=/usr/local/share/vmtests
echo "=== APL";   (cd $T/apl && APL=apl sh tests/run.sh | tail -1)
echo "=== COBOL"; (cd $T/cobol && COBOL=./cobol sh tests/run.sh | tail -1)
echo "=== PLI";   (cd $T/pli && PLI=./pli sh tests/run.sh | tail -1)
echo "=== XBASE"; p=0; f=0
for t in $T/xbase/tests/*.prg; do n=$(basename $t .prg); in=/dev/null
  [ -f $T/xbase/tests/$n.in ] && in=$T/xbase/tests/$n.in
  d=$(mktemp -d); out=$(cd $d && xbase-c $t < $in 2>&1); rm -rf $d
  if [ "$out" = "$(cat $T/xbase/tests/$n.out)" ]; then p=$((p+1)); else f=$((f+1)); echo "FAIL $n"; fi
done; echo "passed $p, failed $f"
echo "=== SMALLTALK"; p=0; f=0
for t in $T/smalltalk/tests/*.st; do n=$(basename $t .st)
  case $n in 16_net|23_unicode) continue;; esac
  in=/dev/null; [ -f $T/smalltalk/tests/$n.in ] && in=$T/smalltalk/tests/$n.in
  fl=; [ -f $T/smalltalk/tests/$n.flags ] && fl=$(cat $T/smalltalk/tests/$n.flags)
  out=$(cd $T/smalltalk && stc -L lib $fl $t < $in 2>&1)
  if [ "$out" = "$(cat $T/smalltalk/tests/$n.out)" ]; then p=$((p+1)); else f=$((f+1)); echo "FAIL $n"; fi
done; echo "passed $p, failed $f"
