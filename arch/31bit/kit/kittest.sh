#!/bin/bash
# kittest.sh BUILD NAME STEPS.json [HELPER...] -- verify kit build BUILD on a
# fresh CE: the dev system is parked as scratchpad/VM370CE.dev, CE is
# unzipped from the pristine upload, every part of the kit over it, the
# Debian and language disks joined, the steps run (esa390mb.sh 256), the
# logs kept in /home/claude/scratch/kittest-NAME/, and the dev system put
# back however the run ends.  HELPER, if given, is run in the background
# with the term log's path as its last argument (e.g. vmspool sends).
# KITNET=1 turns the CTC pair to the PC on (0600, 10.1.1.2).
set -e
R=/home/claude/vm370; S=$R/scratchpad; C=$S/VM370CE.V1.R1.2; P=$S/VM370CE.dev
B=$1; N=$2; STEPS=$(readlink -f $3); shift 3
H=(); for a in "$@"; do [ -e "$a" ] && H+=("$(readlink -f "$a")") || H+=("$a"); done   # helper paths: absolute
pgrep -x hercules >/dev/null && { echo "### hercules is running"; exit 1; }
[ -e $P ] && { echo "### $P exists: a dev system is already parked"; exit 1; }
ls $R/dist/VM370PLUS-kit-*-b$B-part1.zip >/dev/null
mv $C $P
restore() {
  while pgrep -x hercules >/dev/null; do sleep 5; done
  mkdir -p /home/claude/scratch/kittest-$N; cp $C/$N*.log /home/claude/scratch/kittest-$N/ 2>/dev/null || true
  [ -d $P ] && { rm -rf $C; mv $P $C; echo "--- dev system restored"; }
}
trap restore EXIT
cd $S; unzip -q /mnt/user-data/uploads/VM370CE_V1_R1_2.zip; chmod -R u+w $C/disks $C/io
for z in $R/dist/VM370PLUS-kit-*-b$B-part*.zip; do unzip -oq $z -d $S; done
(cd $C; sh joindeb.sh && sh joinlang.sh) | tail -2
# KITNET=1: the CTC pair to the PC (0600, 10.1.1.2) on, as the README tells
# users to do for Linux networking (ssh, NJE, mail from outside)
if [ "$KITNET" = 1 ]; then sed -i 's/^#0600.2  CTCI/0600.2  CTCI/' $C/vm370ce.conf; fi
case "${H[0]:-}" in *.py) H=(python3 "${H[@]}");; esac   # a .py helper needs no exec bit (kt45)
if [ ${#H[@]} -gt 0 ]; then setsid "${H[@]}" $C/$N.term.log > /home/claude/scratch/kittest-$N.helper.out 2>&1 < /dev/null & fi
bash $R/arch/31bit/tools/esa390mb.sh 256 $N $STEPS
