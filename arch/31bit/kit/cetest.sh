#!/bin/bash
# cetest.sh NAME STEPS HELPER -- run steps on a pristine CE (no kit), dev parked
set -e
R=/home/claude/vm370; S=$R/scratchpad; C=$S/VM370CE.V1.R1.2; P=$S/VM370CE.dev
N=$1; STEPS=$(readlink -f $2); HELP=$(readlink -f $3)
pgrep -x hercules >/dev/null && { echo "### hercules is running"; exit 1; }
[ -e $P ] && { echo "### $P exists"; exit 1; }
mv $C $P
restore() {
  while pgrep -x hercules >/dev/null; do sleep 5; done
  mkdir -p /home/claude/scratch/cetest-$N; cp $C/$N*.log /home/claude/scratch/cetest-$N/ 2>/dev/null || true
  [ -d $P ] && { rm -rf $C; mv $P $C; echo "--- dev system restored"; }
}
trap restore EXIT
cd $S; unzip -q /mnt/user-data/uploads/VM370CE_V1_R1_2.zip; chmod -R u+w $C/disks $C/io
grep -n "CNSLPORT\|ARCHMODE\|^00C0\|^000C" $C/vm370ce.conf | head
setsid python3 $HELP $C/$N.term.log > /home/claude/scratch/cetest-$N.helper.out 2>&1 < /dev/null &
python3 $R/arch/31bit/tools/drive.py $C $N $STEPS
