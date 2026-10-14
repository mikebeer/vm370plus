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
| D | DETACH, LOGOFF, HSCH/CSCH, recovery | offline/online in Linux; a guest leaves while the other sends |

## 9.12c — BORDER, and the network configuration

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
