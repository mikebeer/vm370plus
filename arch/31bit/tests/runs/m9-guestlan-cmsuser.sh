#!/bin/bash
# cmsu.sh: CMSUSER joins the LAN (ramdisk Linux from its reader deck), via holdterm's t3.cmd
S=/tmp/claude-0/-home-claude/0e789c93-0fd6-50d9-9e2c-c5456a7467b9/scratchpad
C=$S/t3.cmd
printf '\nlogon cmsuser cmsuser noipl\n' >> $C; sleep 15
printf 'cp def stor 128m\ncp define lcs 650\ncp define lcs 651\ncp set esa on\ncp ipl 00c\n' >> $C; sleep 55
printf 'echo 0.0.0650,0.0.0651 > /sys/bus/ccwgroup/drivers/lcs/group\necho 1 > /sys/bus/ccwgroup/devices/0.0.0650/online\nifconfig eth0 10.3.0.3 netmask 255.255.255.0 up\nsleep 10; ifconfig eth0 | grep packets\n' >> $C; sleep 30
