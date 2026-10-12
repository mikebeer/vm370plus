#!/bin/sh
# mkpascal-cms.sh PASSRC OUT -- the New Stanford Pascal compiler
# (github.com/StanfordPascal/Pascal, Oppolzer, release 2026.10) for CMS
# (VM/370plus 9.13a): PCINT, its P-code interpreter, cross-built with
# m5f/cmscc (31-bit, storage above 16 MB) after pcint-bigendian.patch;
# the compiler and run-time library are the release's own P-code.
#   OUT/pcint.text   PCINT TEXT: LOAD PCINT (CLEAR, GENMOD PCINT
#   OUT/pas.lst      tools/edf.py putlist of PCINT TEXT, the EXECs, the
#                    P-code (PASCAL1 PCODE/PCODE1, PASSCAN, PASLIBX,
#                    PASUTILS), PASCAL MESSAGES and the library sources
set -e
H=$(cd "$(dirname "$0")" && pwd); M=$(cd "$H/../../m5f" && pwd)
S=$(cd "$1" && pwd); O=$(mkdir -p "$2" && cd "$2" && pwd)
if git -C $S apply --check $H/pcint-bigendian.patch 2>/dev/null; then
  git -C $S apply $H/pcint-bigendian.patch; fi
cd $O
for f in pcint pcintcmp oiscach; do
  $M/cmscc -O2 -w -Dmain=pcint_main -c $S/bin/$f.c -o $f.o; done
$M/cmscc -O2 -c $H/cms/pcintcms.c -o pcintcms.o
$M/cmscc -o pcint.elf pcintcms.o pcint.o pcintcmp.o oiscach.o
${E2C:-/home/claude/src/tc/e2c} pcint.elf pcint.text
{
  echo "PCINT TEXT A1 $O/pcint.text BIN"
  echo "PCPAS EXEC A1 $H/cms/PCPAS.EXEC F 80"
  echo "PCRUN EXEC A1 $H/cms/PCRUN.EXEC F 80"
  for f in pascal1.pcode pascal1.pcode1 paslibx.pcode passcan.pcode pasutils.pcode \
           paslibx.pas pasutils.pas passcan.pas; do
    fn=$(echo ${f%%.*} | tr a-z A-Z); ft=$(echo ${f#*.} | tr a-z A-Z)
    echo "$fn $ft A1 $S/src/$f V"; done
  echo "PASCAL MESSAGES A1 $S/etc/pascal.messages V"
  echo "PASTEST PASCAL A1 $H/tests/pastest.pascal F 80"
} > pas.lst
echo "mkpascal-cms: $O/pcint.text, $O/pas.lst"
