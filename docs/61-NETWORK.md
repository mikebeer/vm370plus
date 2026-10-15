# VM/370plus networking plan (9.12, 11 October 2026)

## Today

Each Linux guest has its own CTC pair. Hercules CTCI turns each real pair into a
tun device on the PC, a point-to-point IP link:

| Real pair | Attached to the guest as | PC end | Guest |
|---|---|---|---|
| 0600/0601 | 620/630 | 10.1.1.1 | 10.1.1.2 |
| 0602/0603 | 622/632 | 10.1.2.1 | 10.1.2.2 |
| 06xx/06xx+1 | 62x/63x (x = xx) | 10.1.n.1 | 10.1.n.2, n = xx/2 + 1 |

## 9.12a — per-user addresses (done 11 Oct, I-266)

The guest's address follows its CTC pair, so every copy of the Debian disk works
for every user, with no per-user file:

- On the Debian disks, `/usr/local/sbin/vm370plus-net` runs from `rc.local`. It
  readdresses ctc0 when the initramfs's address (taken from the disk's IPL
  parameters) is not the one for the pair. It is also written by `vm370plus-post.sh`.
- The initramfs (`init.sh`) uses the same rule from the next kernel build on.
- The pair can be attached as 62x/63x or at its real address. Two CTCs must be on
  different virtual control units (620 and 630, not 620 and 621); I-269 was the CP
  crash when that rule was broken.

## Why not DHCP over the CTCs

DHCP is a broadcast on a shared Ethernet segment. A CTC link is point-to-point at
the IP level, so a DHCP server in a service machine would have nobody to hear it.
DHCP inside VM/370plus needs a shared segment first (9.12b).

## 9.12b — guest LAN

CP gives each guest a virtual NIC on one internal segment, like z/VM's Guest LAN.
This is CP work and the largest step of 9.12.

### Why LCS, and why simulated by CP (14 Oct)

- QETH (z/VM's Guest LAN device) is QDIO: SIGA, shared queues, adapter
  interrupts -- far more machinery than an ESA/390 CP has. LCS (3088 model 60,
  Linux's `lcs` driver, kernel #11 has it) is a channel-program device.
- The intermediate step, a real Hercules LCS dedicated to a guest and bridged on
  the PC (tap0/tap1 in br9), fails (I-291): Linux's LCS driver runs its read and
  write rings with CCW **suspend** flags, **PCI** and **RSCH**, and changes the
  suspend flags in its own storage between resumes. CP translates a dedicated
  device's channel program once into a real copy and has neither suspend nor
  RSCH. A device that CP simulates reads the guest's own CCWs as it goes, so
  suspend, resume and PCI are natural there. Kept from the step: kernel #11
  (CONFIG_LCS), and dedicated devices now answer SENSE ID themselves.

### The design

- `DEFINE LCS vdev [LAN name]` (default LAN `SYSTEM`) defines the pair vdev
  (read, even) and vdev+1 (write): class special, a new type TYPLCS. SENSE ID
  answers CU 3088 model 60 (SIDTAB). `DETACH vdev` leaves the LAN.
- Each LAN member has a MAC address made by CP: 02-00-00-uu-uu-vv (locally
  administered; uuuu from the user's VMBLOK slot, vv from vdev).
- **Write device.** At SSCH/RSCH CP walks the guest's format-1 CCWs from the
  current one: each WRITE's buffer holds LCS frames (header: offset of the next
  frame, type, slot; offset 0 ends the buffer). Type 0 is a control command --
  STARTUP, STARTLAN, LANSTAT (CP answers with the member's MAC), STOPLAN,
  SHUTDOWN, QIPASSIST (no assists), SETIPM/DELIPM (multicast: accepted) -- and
  its reply (same frame, return code 0) is queued for the read device. Type 1
  is an Ethernet frame: CP switches it -- to the member with that destination
  MAC, or to every other member for broadcast and multicast.
  A CCW with the suspend flag suspends the subprogram (SCSW activity
  "suspended", status to the guest as the channel subsystem would give it);
  RSCH resumes at that CCW, which the guest has meanwhile changed. TIC and
  command chaining as usual.
- **Read device.** Received frames wait in the member's queue (CP free
  storage, bounded: drop when full, as a LAN does). While the read program is
  active, CP fills the current READ CCW's buffer with as many frames as fit
  (each with its LCS header, then a terminating offset 0), advances, and
  presents intermediate status when that CCW had PCI; a CCW with the suspend
  flag suspends. With nothing queued the read waits (no status), as a real LCS
  read does.
- HSCH/CSCH end the subprogram at once (the driver uses them at offline and
  recovery).
- Interrupts and status go through DMKVCS's TSCH/IRB path, which needs the
  SCSW's activity-control bits for suspend and the PCI intermediate status.
- Later (9.12c): one LAN member may be an uplink -- a Hercules tap through a
  dedicated device once CP can carry it, or BORDER as the only member with two
  NICs.

### Steps

| Step | What | Test |
|---|---|---|
| A | DEFINE LCS, TYPLCS, SENSE ID 3088-60 | Linux binds `lcs` to the pair -- **done 14 Oct (la1)**: `CP DEFINE LCS 640`/`641` (DEV 640 DEFINED), Linux sees 3088/01 on 3088/60, driver lcs, `group` makes 0.0.0640. Until step B the pair is an uncoupled adapter (not ready, DMKVCAST/DMKDIBSM). |
| B | write and read programs, suspend/RSCH/PCI, control commands | `eth0` up with CP's MAC (`ifconfig`) -- **done 14 Oct (lb3/lb4)**: STARTUP, STARTLAN, LANSTAT (the reply is 64 bytes; Linux copies the MAC from it), QIPASSIST; MAC 02-vv-vv-vv-dd-dd (VMBLOK, device). |
| C | the switch: unicast, broadcast, queues | two guests ping each other (ARP over the LAN) -- **done 14 Oct (lb6)**: MAINT and CMSUSER, each a ramdisk Linux (kernel #11 from the reader), 10.3.0.2 and 10.3.0.3; ARP, ping, ping -s 1400 (19 of 20, 25-900 ms). Frames for another member are queued; its read side gets a PCI interruption with the CPA unchanged (Linux takes no buffer) and its own TSCH pumps the frames -- no cross-user TRANS, no CPEXBLOK (I-292). Test `tests/runs/m9-guestlan-2ram.json` with a second terminal (holdterm) for CMSUSER. |
| D | DETACH, LOGOFF, HSCH/CSCH, recovery | offline/online in Linux; a guest leaves while the other sends -- **done 14 Oct (lb8)**: MAINT pings CMSUSER every 2 s (`ping -i 2 -c 600 10.3.0.3 &`); CMSUSER `#cp logoff` (CP stays up), logs on again, rejoins (pings answered again), takes eth0 offline and online (HSCH/CSCH), pings back, then `#cp detach 650` and `651` under traffic. VLVALID checks every destination before a frame is queued or a member kicked: its user must be in the VMBLOK chain (round once from the system VMBLOK's VMPNT -- the system VMBLOK is not in the cycle; the first version looped, lb7) and its read device must still be an LCS (DMKSCNVU); a stale member's frames are freed and its slot cleared. Test `tests/runs/m9-guestlan-stepd.json` + `m9-guestlan-cmsuser.sh`. |

### Debian on the LAN (14 Oct, lb9)

`/usr/local/sbin/vm370plus-lan` (from `rc.local`, written by
`arch/31bit/debian/vm370plus-post.sh`, installed on the dev Debian disk):
finds the even subchannel of a 3088/60 pair, groups it with the next one,
sets it online and gives eth0 the address for `vmcp q userid` in
`/etc/vm370plus-lan.conf` (MAINT 10.3.0.2, CMSUSER 10.3.0.3), else
10.3.0.(n+1) for the CTC pair n. MAINT Debian (kernel #11, `CP DEFINE LCS
640/641` before `CP IPL LINUX`) and CMSUSER's ramdisk Linux: ping both
ways, a TCP connect to Debian's sshd, and 200,000 bytes by nc from CMSUSER
to Debian in 5.4 s with the same MD5 (about 37 KB/s).

## 9.12c — BORDER (done 15 Oct, runs bor1, bi1, bi2)

BORDER is the guest LAN's only way out. It is a service machine (user BORDER)
running the in-storage Linux with kernel #12 (kernel #11 + netfilter; Debian
7's iptables 1.4.14 and its libraries in the initramfs):

- **ctc0** to the PC on the CTC pair **0606/0607**, DEDICATEd as 626/636:
  10.1.4.2, the PC end 10.1.4.1 (`0606.2 CTCI 10.1.4.2 10.1.4.1`). (0608 is not
  in CP's real I/O configuration.)
- **eth0** on the guest LAN: CP DEFINE LCS 640/641, 10.3.0.1.
- `ip_forward`, MASQUERADE out of ctc0; FORWARD drops all but
  established/related, LAN to outside, and DNATed connections; INPUT from ctc0
  allows established, ssh (22) and ICMP, drops the rest.
- Port forwards from **BORDER CONFIG** on MAINT 19E (mode 2: other users see
  19E as Y/S, which shows only mode-2 files): `EXPOSE TCP 2223 TO 10.3.0.3 22`.
  BORDER EXEC turns them into `vmfwd=tcp:2223:10.3.0.3:22,...` kernel
  parameters (one `vmfwd=` per parameter card; the kernel reads the whole parm
  file, up to 896 bytes).
- The initramfs `/init` (all users): an LCS pair becomes eth0 with `vmlan=` or
  10.3.0.(n+1) for CTC pair n; a guest without a CTC gets the default route
  10.3.0.1 and dropbear. Debian's `vm370plus-lan` adds the same default route
  when it has none.

**The BORDER user**: `IPL CMS`, 64M, a 20-cylinder 191 on VM50U1 (71-90) with
LINUX12 KERNEL1/KERNEL2 (one kernel; a classic CMS file has at most 65,533
records), BORDER EXEC and a PROFILE EXEC (`EXEC BORDER`). Start it with
`CP AUTOLOG BORDER BORDER` (the operator sees `BORDER: starting` and `IPL of
its Linux`); to stay in CMS, logon with `ACCESS (NOPROF` as the first command.
The directory entry was added by DIRUP2/DIRUP3 EXEC
(`linux390/border/`). Why classic CMS: under `IPL 290` the machine first stops
in a CP read after the IPL, so AUTOLOG's first command goes to CP, not CMS
(I-295); the classic CMS runs the profile, and needs no first command.

Tested (bi2): AUTOLOG BORDER brings it up; CMSUSER's in-storage Linux on the
LAN (10.3.0.3, default route 10.3.0.1) pings the PC 10.1.4.1 through the NAT
(ttl 63); `ssh -p 2223 root@10.1.4.2` from the PC lands on CMSUSER's guest;
ports not exposed (80, 2224) time out; ssh to BORDER itself works.

On the PC: 10.1.4.1 is the PC's end of BORDER's CTC; for the internet, the PC
routes and NATs 10.1.4.2 as it does for the other CTC addresses; to expose a
service to the PC's own LAN, forward a PC port to 10.1.4.2:port (netfwd.py).

Later: the configuration-file ideas below (OUTSIDE DHCP / a bridged LCS on a
tap) are not done; BORDER's outside is the CTC.

### Original 9.12c design notes (13 Oct)

BORDER is the only machine with a link to the PC's network. It does three things:

- NAT for all outgoing traffic;
- port forwarding from outside to service machines (httpd from 9.5, and others);
- a firewall: nothing comes in that is not exposed.

One configuration file on MAINT, read by BORDER and CP, holds the settings:

```
OUTSIDE DHCP                         * address from the PC's LAN (bridged: LCS on tap)
OUTSIDE STATIC 192.168.1.50/24 192.168.1.1   * or a fixed address
INSIDE  10.1.0.0/16                  * the internal network
ADDRESS FIXED | DHCP                 * per CTC pair (today), or by 9.12d's server
EXPOSE  TCP 80   TO HTTPD 80         * targets by user ID ...
EXPOSE  TCP 2222 TO 10.1.2.2 22      * ... or by address
```

`OUTSIDE DHCP` needs a bridged Hercules link (LCS on a tap device) so that BORDER
sits on the LAN itself. `OUTSIDE STATIC` also works with the routed tun link.

## 9.12d — DHCP and DNS inside

A service machine runs DHCP and DNS for the internal segment: dnsmasq on Debian,
or BORDER itself. It needs only the inside segment, not the outside.
