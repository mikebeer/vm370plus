#!/bin/sh
# install.sh -- put the spool bridge (9.25) on the Debian guest:
#   sh install.sh        (as root, in this directory)
set -e
install -m 755 vmspoold /usr/local/sbin/vmspoold
mkdir -p /etc/vmspool.d /var/spool/vmspool
for h in handlers/*; do install -m 755 $h /etc/vmspool.d/; done
install -m 755 vmspoold.init /etc/init.d/vmspoold
[ -r /etc/default/vmspoold ] || echo 'VMSPOOLD_OPTS="-t 900"' > /etc/default/vmspoold
update-rc.d vmspoold defaults >/dev/null
/etc/init.d/vmspoold restart
