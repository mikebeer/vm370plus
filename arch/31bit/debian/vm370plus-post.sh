#!/bin/sh
# VM/370plus: finish a Debian wheezy s390 system after debootstrap's second
# stage (run inside the chroot).  Console: the 3215 is ttyS0, line mode.
set -e
echo vm370plus > /etc/hostname
cat > /etc/hosts <<H
127.0.0.1 localhost
10.1.1.2  vm370plus
H
cat > /etc/fstab <<F
# VM/370plus: the root is mounted by the initramfs before switch_root
/dev/dasda1  /      ext2   defaults,errors=remount-ro  0  0
proc         /proc  proc   defaults                    0  0
/swapfile    none   swap   sw                          0  0
F
# 64 MB is too little for dpkg's xz on some packages: 128 MB of swap
[ -f /swapfile ] || { dd if=/dev/zero of=/swapfile bs=1M count=128; mkswap /swapfile; }
chmod 600 /swapfile
cat > /etc/network/interfaces <<N
auto lo
iface lo inet loopback

# the CTC pair to the PC (Hercules CTCI).  The initramfs groups the
# adapters and configures ctc0 from the IPL parameters (vmip=, vmpeer=;
# default 10.1.1.2 / 10.1.1.1) before switch_root, so the same disk
# works for every user; Debian leaves it alone.
auto ctc0
iface ctc0 inet manual
N
echo "nameserver 8.8.8.8" > /etc/resolv.conf
cat > /etc/apt/sources.list <<S
deb http://archive.debian.org/debian wheezy main
S
echo 'Acquire::Check-Valid-Until "false";' > /etc/apt/apt.conf.d/10archive
# the console: one getty on the 3215 (line mode, TERM dumb), no VTs
sed -i -e '/^[1-6]:/d' -e '/^T[0-9]:/d' /etc/inittab
echo 'T0:2345:respawn:/sbin/getty -L ttyS0 9600 dumb' >> /etc/inittab
grep -q '^ttyS0$' /etc/securetty || echo ttyS0 >> /etc/securetty
echo 'root:vm370plus' | chpasswd
# sshd: allow the root password login, as on the RAM system
sed -i 's/^PermitRootLogin .*/PermitRootLogin yes/' /etc/ssh/sshd_config
# no hardware clock, no keyboard, no udev settle wait at boot
echo 'HWCLOCKACCESS=no' >> /etc/default/rcS 2>/dev/null || true
# debootstrap's own clean-up, if its second stage did not get that far
[ -x /sbin/start-stop-daemon.REAL ] && mv /sbin/start-stop-daemon.REAL /sbin/start-stop-daemon
rm -f /usr/sbin/policy-rc.d
# ctc0's address from the CTC pair the machine runs on (I-266, 9.12a)
cat > /usr/local/sbin/vm370plus-net <<'NET'
#!/bin/sh
# VM/370plus: give ctc0 the address that belongs to its CTC pair (I-266).
# The PC side of real CTC pair 06xx (Hercules CTCI) is 10.1.n.1 with
# n = xx/2 + 1: 0600 -> 10.1.1.2/.1, 0602 -> 10.1.2.2/.1, ...  Attach
# the pair as 62x (read) and 63x (write), x its offset in the 06xx block
# (600/601 as 620/630, 602/603 as 622/632; DEDICATE 622 602 and 632 603
# in the directory), or at its real address.  The initramfs set ctc0 from the IPL parameters, which
# are the same on every copy of the disk; this corrects it.  Run from
# /etc/rc.local.  ctc0 is never taken down (a CTC restart hangs).
d=$(readlink /sys/class/net/ctc0/device 2>/dev/null) || exit 0
dev=${d##*.}
case $dev in
  062?) n=$(( (0x$dev - 0x620) / 2 + 1 ));;
  06[01]?) n=$(( (0x$dev - 0x600) / 2 + 1 ));;
  *) exit 0;;
esac
ip=10.1.$n.2; peer=10.1.$n.1
cur=$(ifconfig ctc0 | sed -n 's/.*inet addr:\([0-9.]*\).*/\1/p')
[ "$cur" = "$ip" ] && exit 0
ifconfig ctc0 $ip pointopoint $peer mtu 1500 up
route del default 2>/dev/null
route add default gw $peer
sed -i "s/^10\.1\.[0-9]*\.2 /$ip /" /etc/hosts
echo "vm370plus-net: ctc0 $ip (peer $peer), from CTC $dev"
NET
chmod 755 /usr/local/sbin/vm370plus-net
grep -q vm370plus-net /etc/rc.local || sed -i 's|^exit 0|/usr/local/sbin/vm370plus-net\nexit 0|' /etc/rc.local
cat > /usr/local/sbin/vm370plus-lan <<'LAN'
#!/bin/sh
# VM/370plus: eth0 on the CP guest LAN (9.12b).  CP DEFINE LCS 640 and
# CP DEFINE LCS 641 (before the IPL) give this machine a CP-simulated
# 3088-60 LCS on the one internal segment 10.3.0.0/24; Linux's lcs
# driver makes it eth0.  The address: /etc/vm370plus-lan.conf (USERID
# ADDRESS lines) for this user, else 10.3.0.(n+1) for the CTC pair n of
# vm370plus-net (MAINT 10.3.0.2, CMSUSER 10.3.0.3), else none.
# Run from /etc/rc.local.
r=
for d in /sys/bus/ccw/devices/*; do
  [ "$(cat $d/cutype 2>/dev/null)" = 3088/60 ] || continue
  b=${d##*/}; x=$(( 0x${b##*.} ))
  [ $(( x % 2 )) = 0 ] && { r=$b; break; }
done
[ -n "$r" ] || exit 0
w=$(printf '0.0.%04x' $(( 0x${r##*.} + 1 )))
[ -e /sys/bus/ccwgroup/devices/$r ] || echo $r,$w > /sys/bus/ccwgroup/drivers/lcs/group
echo 1 > /sys/bus/ccwgroup/devices/$r/online 2>/dev/null
sleep 2
u=$(vmcp q userid 2>/dev/null | awk '{print $1}')
ip=
[ -n "$u" ] && [ -r /etc/vm370plus-lan.conf ] && ip=$(awk -v u="$u" 'toupper($1)==toupper(u) {print $2}' /etc/vm370plus-lan.conf)
if [ -z "$ip" ]; then
  c=$(readlink /sys/class/net/ctc0/device 2>/dev/null); c=${c##*.}
  case $c in
    062?) ip=10.3.0.$(( (0x$c - 0x620) / 2 + 2 ));;
    06[01]?) ip=10.3.0.$(( (0x$c - 0x600) / 2 + 2 ));;
  esac
fi
[ -n "$VMLAN_DHCP" ] && ip=
if [ -z "$ip" ]; then
  # 9.12d: no address of its own -- ask BORDER's DHCP server
  ifconfig eth0 up
  if command -v dhclient >/dev/null && dhclient -1 -pf /run/dhclient.eth0.pid -lf /var/lib/dhcp/dhclient.eth0.leases eth0; then
    echo "vm370plus-lan: eth0 $(ifconfig eth0 | sed -n 's/.*inet addr:\([0-9.]*\).*/\1/p') from DHCP (BORDER)"
  else
    echo "vm370plus-lan: eth0 up, no address for ${u:-this user} (see /etc/vm370plus-lan.conf; DHCP needs BORDER)"
  fi
  exit 0
fi
ifconfig eth0 $ip netmask 255.255.255.0 up
# no CTC of its own: out through BORDER (9.12c)
route -n | grep -q "^0\.0\.0\.0" || { route add default gw 10.3.0.1; grep -q "^nameserver" /etc/resolv.conf || echo "nameserver 10.3.0.1" >> /etc/resolv.conf; }
echo "vm370plus-lan: eth0 $ip on the guest LAN (LCS $r)"
LAN
chmod 755 /usr/local/sbin/vm370plus-lan
[ -e /etc/vm370plus-lan.conf ] || printf '# USERID ADDRESS on the CP guest LAN (10.3.0.0/24)\nMAINT 10.3.0.2\nCMSUSER 10.3.0.3\n' > /etc/vm370plus-lan.conf
grep -q vm370plus-lan /etc/rc.local || sed -i 's|^exit 0|/usr/local/sbin/vm370plus-lan\nexit 0|' /etc/rc.local
# I-293: halt -i (Debian's halt script takes every interface down) hangs
# with eth0 on the CP guest LAN; ifconfig eth0 down alone works.
grep -q "^NETDOWN" /etc/default/halt || printf '# VM/370plus: halt -i hangs with eth0 on the CP guest LAN (I-293)\nNETDOWN=no\n' >> /etc/default/halt
echo "VM/370plus post-install done"
