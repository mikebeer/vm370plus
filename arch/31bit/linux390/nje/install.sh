#!/bin/sh
# install.sh -- VM/370plus 9.13b: NJE-II on the Debian guest (as root, in
# a directory with nje-s390.tgz (mknje.sh s390), njeconf.sh, njefile,
# njemsg, njecards and nje.init).  The node: NODE (VM370CE, as CMS's
# SYSTEM NETID), IP (10.1.1.2), PEERS (PCNJE:10.1.1.1, a node on the PC);
# an existing /usr/local/nje/etc/nje.cf is kept unless FORCE=1.
#   later: njeconf /usr/local/nje NODE IP PEER:HOST[:PORT] ...
set -e
N=/usr/local/nje
tar xzf nje-s390.tgz -C /
install -m 755 njefile njemsg njecards $N/lib/
install -m 755 njeconf.sh $N/bin/njeconf
if [ ! -r $N/etc/nje.cf ] || [ "$FORCE" = 1 ]; then
  sh njeconf.sh $N ${NODE:-VM370CE} ${IP:-10.1.1.2} ${PEERS:-PCNJE:10.1.1.1}
fi
mkdir -p /var/spool/vmspool/nje
install -m 755 nje.init /etc/init.d/nje
update-rc.d nje defaults >/dev/null
/etc/init.d/nje restart
