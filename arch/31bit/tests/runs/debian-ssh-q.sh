#!/bin/sh
# q.sh "command" -- run in Debian through sh.py, wait, print
Q=/home/claude/scratch/mdb/q; n=$(ls $Q/*.cmd 2>/dev/null | wc -l); f=$(printf '%s/%03d' $Q $((n+1)))
printf '%s\n' "$*" > $f.cmd
until [ -f $f.out ]; do sleep 1; done; cat $f.out
