#!/bin/sh
# mkvmdns.sh -- build border/vmdns (9.40) for BORDER's initramfs: 31-bit,
# dynamic against Debian 7's libc 2.13 (in the initramfs for iptables), so
# it stays small (BORDER's 191 has no room for a static one)
cd "$(dirname "$0")"
W=/home/claude/src/cobol/wsys     # a Debian 7 s390 sysroot
s390x-linux-gnu-gcc -m31 -march=z900 -Os -no-pie -fno-pie -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 \
  -fno-stack-protector --sysroot=$W -B$W/usr/lib/s390-linux-gnu -L$W/usr/lib/s390-linux-gnu \
  -L$W/lib/s390-linux-gnu -Wno-format-truncation -o vmdns vmdns.c && s390x-linux-gnu-strip vmdns && ls -l vmdns
