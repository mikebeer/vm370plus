#!/bin/sh
# /init for Linux/390 on VM/370plus (BusyBox userland, in memory)
/bin/busybox mkdir -p /proc /sys /tmp /dev /root /var/log /usr/bin /usr/sbin /sbin
/bin/busybox mount -t proc proc /proc
/bin/busybox mount -t sysfs sysfs /sys
/bin/busybox mount -t tmpfs tmpfs /tmp 2>/dev/null
/bin/busybox --install -s
# PID 1 is this script, which ignores the signals busybox poweroff/halt/reboot
# send to init, so each acts at once (Linux stops in a disabled wait)
for c in poweroff halt reboot; do rm -f /sbin/$c
  printf '#!/bin/sh\nsync\numount -a -r 2>/dev/null\nexec /bin/busybox %s -f\n' $c > /sbin/$c; chmod 755 /sbin/$c; done
hostname vm370plus
mount -t devtmpfs devtmpfs /dev 2>/dev/null
mkdir -p /dev/pts /etc/dropbear; mount -t devpts devpts /dev/pts
# Disk: every 3390 (or 3380) attached to this virtual machine goes
# online; the first one becomes /data, an ext2 file system that keeps
# its files across IPLs (made on first use -- e.g. a Hercules volume
# from  dasdinit -z -linux file.cckd 3390-1 LNX001 ,  CP ATTACH'ed).
for d in /sys/bus/ccw/devices/*; do
  case "$(cat $d/devtype 2>/dev/null)" in
    3390/*|3380/*) echo 1 > $d/online 2>/dev/null;; esac; done
sleep 1
# VM/370plus converts a format-1 channel program of at most 64 CCWs (D-21):
# 128 KB per request is 32 records of 4 KB, plus Define Extent and
# Locate Record -- well inside.
for q in /sys/block/dasd*/queue/max_sectors_kb; do [ -e $q ] && echo 128 > $q; done
# a volume without a Linux partition gets one over all of it (mkdasdpart)
for d in /dev/dasd?; do
  [ -b $d ] && [ ! -b ${d}1 ] || continue
  mkdasdpart $d && blockdev --rereadpt $d && sleep 1
done
for b in /dev/dasd?1; do
  [ -b $b ] || continue
  mkdir -p /data
  mount -t ext2 $b /data 2>/dev/null || {
    echo "disk: $b has no file system yet -- making one (once)"
    mke2fs -q -b 4096 $b && mount -t ext2 $b /data; }
  if mountpoint -q /data; then echo "disk: $b on /data (persistent)"; break; fi
done
ifconfig lo 127.0.0.1 up
# Network: a CTC pair (Hercules CTCI) attached to this virtual machine, e.g.
#   CP ATTACH 600 * 620  and  CP ATTACH 601 * 621  (the lower one reads).
# Addresses from the IPL parameters vmip= vmpeer= (default 10.1.1.2, and
# the PC at the other end of the CTC, 10.1.1.1).
VMIP=10.1.1.2; VMPEER=10.1.1.1
for a in $(cat /proc/cmdline); do case $a in
  vmip=*) VMIP=${a#vmip=};; vmpeer=*) VMPEER=${a#vmpeer=};; esac; done
CTC=""
for d in /sys/bus/ccw/devices/*; do
  [ "$(cat $d/cutype 2>/dev/null)" = 3088/08 ] && CTC="$CTC ${d##*/}"; done
set -- $CTC
if [ $# -ge 2 ]; then
  # I-266: the PC side of real CTC pair 06xx (Hercules CTCI) is 10.1.n.1,
  # n = xx/2 + 1 (0600 -> 10.1.1.x, 0602 -> 10.1.2.x).  The pair is
  # attached as 62x (read) and 63x (write), x being its offset in the
  # 06xx block (600/601 as 620/630, 602/603 as 622/632), or at its real
  # address; either names this machine's address.  vmip=/vmpeer= are for
  # other addresses.  Every user's copy of the disk then works.
  dev=${1##*.}
  case $dev in 062?) n=$(( (0x$dev - 0x620) / 2 + 1 ));;
    06[01]?) n=$(( (0x$dev - 0x600) / 2 + 1 ));; *) n=;; esac
  [ -n "$n" ] && { VMIP=10.1.$n.2; VMPEER=10.1.$n.1; }
  echo $1,$2 > /sys/bus/ccwgroup/drivers/ctcm/group
  echo 1 > /sys/bus/ccwgroup/drivers/ctcm/$1/online
  ifconfig ctc0 $VMIP pointopoint $VMPEER mtu 1500 up && route add default gw $VMPEER
  dropbear -R -p 22 && echo "network: ctc0 $VMIP (peer $VMPEER), ssh: root / vm370plus"
else
  echo "network: none (attach a CTC pair, e.g. CP ATTACH 600 * 620, 601 * 621)"
fi
# Guest LAN (9.12b): a CP-simulated LCS pair (CP DEFINE LCS 640 and 641)
# becomes eth0 on the one segment 10.3.0.0/24.  Its address: vmlan= from
# the IPL parameters, else 10.3.0.(n+1) for the CTC pair n above, else
# 10.3.0.1 for BORDER (vmrole=border).  Without a CTC the default route
# is BORDER, 10.3.0.1 (9.12c).
ROLE=; LANIP=; FWD=; DNS=
for a in $(cat /proc/cmdline); do case $a in
  vmrole=*) ROLE=${a#vmrole=};; vmlan=*) LANIP=${a#vmlan=};; vmfwd=*) FWD="$FWD,${a#vmfwd=}";; vmdns=*) DNS=${a#vmdns=};; esac; done
LAN=
for d in /sys/bus/ccw/devices/*; do
  [ "$(cat $d/cutype 2>/dev/null)" = 3088/60 ] || continue
  b=${d##*/}; [ $(( 0x${b##*.} % 2 )) = 0 ] && { LAN=$b; break; }
done
if [ -n "$LAN" ]; then
  w=$(printf '0.0.%04x' $(( 0x${LAN##*.} + 1 )))
  echo $LAN,$w > /sys/bus/ccwgroup/drivers/lcs/group
  echo 1 > /sys/bus/ccwgroup/devices/$LAN/online 2>/dev/null
  sleep 2
  [ "$ROLE" = border ] && LANIP=${LANIP:-10.3.0.1}
  [ -z "$LANIP" ] && [ -n "$n" ] && LANIP=10.3.0.$((n + 1))
  if [ -n "$LANIP" ]; then
    ifconfig eth0 $LANIP netmask 255.255.255.0 up
    echo "network: eth0 $LANIP on the guest LAN"
    [ $# -ge 2 ] || { route add default gw 10.3.0.1 2>/dev/null
      echo "nameserver 10.3.0.1" > /etc/resolv.conf; dropbear -R -p 22; }
  else
    # 9.12d: no address of its own -- ask BORDER's DHCP server
    ifconfig eth0 up
    udhcpc -i eth0 -n -q -t 5 -T 3 -s /etc/udhcpc.script 2>/dev/null ||
      echo "network: eth0 on the guest LAN, no address (vmlan=, or BORDER's DHCP)"
    [ $# -ge 2 ] || dropbear -R -p 22
  fi
fi
# BORDER (9.12c): the LAN's only way out.  It routes the LAN to the CTC
# (the PC), hides it behind its own CTC address (NAT), lets nothing in
# but the forwards in vmfwd=proto:port:address:port[,...] (vmfwd= may
# come more than once: one per parameter card), and ssh to
# itself.  iptables is Debian 7's (1.4.14) with its libraries.
if [ "$ROLE" = border ] && [ -n "$LAN" ] && [ $# -ge 2 ]; then
  export LD_LIBRARY_PATH=/lib/s390-linux-gnu:/lib
  echo 1 > /proc/sys/net/ipv4/ip_forward
  ipt() { /sbin/iptables "$@" || echo "border: iptables $* failed"; }
  ipt -P FORWARD DROP
  ipt -A FORWARD -m conntrack --ctstate ESTABLISHED,RELATED -j ACCEPT
  ipt -A FORWARD -i eth0 -o ctc0 -j ACCEPT
  ipt -A FORWARD -i ctc0 -o eth0 -m conntrack --ctstate DNAT -j ACCEPT
  ipt -A INPUT -i ctc0 -m conntrack --ctstate ESTABLISHED,RELATED -j ACCEPT
  ipt -A INPUT -i ctc0 -p tcp --dport 22 -j ACCEPT
  ipt -A INPUT -i ctc0 -p icmp -j ACCEPT
  ipt -A INPUT -i ctc0 -j DROP
  ipt -t nat -A POSTROUTING -o ctc0 -j MASQUERADE
  for f in $(echo "$FWD" | tr ',' ' '); do
    IFS=: read pr po ad dp <<EOT
$f
EOT
    ipt -t nat -A PREROUTING -i ctc0 -p $pr --dport $po -j DNAT --to-destination $ad:${dp:-$po}
    echo "border: $VMIP $pr $po -> $ad:${dp:-$po}"
  done
  # 9.12d: DHCP for the LAN (10.3.0.100-199; the router and the name
  # server are BORDER), and names: 10.3.0.1:53 is passed on to vmdns=
  # (default the PC, 10.1.4.1)
  DNS=${DNS:-$VMPEER}
  printf 'start 10.3.0.100\nend 10.3.0.199\ninterface eth0\nlease_file /tmp/udhcpd.leases\nopt router 10.3.0.1\nopt dns 10.3.0.1\nopt subnet 255.255.255.0\nopt lease 86400\n' > /etc/udhcpd.conf
  : > /tmp/udhcpd.leases
  udhcpd /etc/udhcpd.conf && echo "border: DHCP on eth0, 10.3.0.100-199"
  for p in udp tcp; do
    ipt -t nat -A PREROUTING -i eth0 -d 10.3.0.1 -p $p --dport 53 -j DNAT --to-destination $DNS
  done
  echo "nameserver $DNS" > /etc/resolv.conf
  echo "border: names (10.3.0.1:53) from $DNS"
  hostname border
  echo "border: NAT for 10.3.0.0/24 out through ctc0 $VMIP; ip_forward on"
fi
# A Linux system installed on the disk (Debian, see debian-install): boot
# it, unless the IPL parameters say vmroot=ram.  The CTC group and the
# disk stay as set up here; the system on the disk configures the rest.
if [ -x /data/sbin/init ] && [ ! -d /data/debootstrap ] && ! grep -q vmroot=ram /proc/cmdline; then
  echo "root: the system on $(mount | awk '$3=="/data"{print $1}') -- switch_root (vmroot=ram stays here)"
  killall dropbear 2>/dev/null
  # ctc0 stays as configured here (vmip/vmpeer from the IPL parameters,
  # so every user's copy of the disk is the same); Debian has it
  # "manual".  Not down: a CTC restart hangs.  No colours on the line
  # console.
  export TERM=dumb
  umount /dev/pts /tmp /proc /sys 2>/dev/null
  mount --move /dev /data/dev 2>/dev/null || umount /dev 2>/dev/null
  exec switch_root /data /sbin/init
fi
echo
echo "HELLO FROM LINUX/390 ON VM/370plus (M7)"
cat /etc/motd
uname -a
echo "BusyBox userland (in memory): ls /bin  ps  free  top -n1  vi  dmesg  poweroff"
export TERM=dumb HOME=/root PS1='\h:\w# ' PATH=/bin:/sbin:/usr/bin:/usr/sbin
cd /root
while true; do /bin/sh; echo "(shell ended -- restarting; 'poweroff' stops Linux)"; done
