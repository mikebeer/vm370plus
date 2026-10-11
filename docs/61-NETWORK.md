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

CP gives each guest a virtual NIC, LCS or QETH style, on one internal segment,
like z/VM's Guest LAN or VSWITCH. This is CP work and the largest step of 9.12.

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
