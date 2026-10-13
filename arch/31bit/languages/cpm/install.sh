#!/bin/sh
# install.sh -- CP/M 2.2 (RunCPM) in the Debian guest (9.33), as root here
set -e
L=/usr/local/lib/cpm
mkdir -p $L/A /var/lib/cpm
install -m 755 RunCPM $L/RunCPM
install -m 644 CCP-DR.60K CCP-DR.64K $L/
rm -rf $L/A/0; cp -r A0 $L/A/0
install -m 755 cpm.handler /etc/vmspool.d/cpm
install -m 755 runcpm /usr/local/bin/runcpm
