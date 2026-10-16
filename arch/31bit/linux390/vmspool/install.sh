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
# 9.14 TELL: tell (CP MSG from Linux, and the messages from CMS) and the
# setuid helper vmmsg, which only ever runs CP MSG
install -m 755 linux/tell /usr/local/bin/tell
install -m 4755 -o root linux/vmmsg /usr/local/sbin/vmmsg
mkdir -p /var/spool/vmspool/msg
# 9.42 Markdown: md2ms (Markdown to troff ms or plain text), mdprint
install -m 755 linux/md2ms /usr/local/bin/md2ms
install -m 755 linux/mdprint /usr/local/bin/mdprint
# 9.18 mail: vmmail.py (shared), vmmaild (SMTP in) and the service mail
mkdir -p /usr/local/lib/vmspool /var/spool/vmspool/outq /var/spool/vmspool/mail
install -m 644 linux/vmmail.py /usr/local/lib/vmspool/vmmail.py
install -m 755 linux/vmmaild /usr/local/sbin/vmmaild
[ -r /etc/vmspool.d/mail.conf ] || install -m 600 linux/mail.conf /etc/vmspool.d/mail.conf
install -m 755 linux/vmmaild.init /etc/init.d/vmmaild
update-rc.d vmmaild defaults >/dev/null
/etc/init.d/vmmaild restart
