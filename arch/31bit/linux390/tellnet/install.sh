#!/bin/sh
# install.sh -- TELL to and from the internet (9.32), as root in the guest
set -e
install -m 755 tellnet.handler /etc/vmspool.d/tellnet
install -m 644 tellnet.conf /etc/vmspool.d/tellnet.conf
install -m 755 tellgw /usr/local/sbin/tellgw
install -m 755 tellnet-user /usr/local/sbin/tellnet-user
install -m 755 tellgw.init /etc/init.d/tellgw
install -m 644 acl /etc/mosquitto/acl
[ -f /etc/mosquitto/passwd ] || install -m 600 /dev/null /etc/mosquitto/passwd
cp /etc/mosquitto/mosquitto.conf /etc/mosquitto/mosquitto.conf.9-25
install -m 644 mosquitto.conf /etc/mosquitto/mosquitto.conf
update-rc.d tellgw defaults >/dev/null
pkill -x mosquitto || true; sleep 1
mosquitto -d -c /etc/mosquitto/mosquitto.conf
/etc/init.d/tellgw restart
