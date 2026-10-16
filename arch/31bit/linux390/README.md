# Linux 4.0 31-bit for M7 (ESA/390 guest under VM/370plus)

Built in the workspace from the v4.0 tag with `s390x-linux-gnu-gcc` 13:

    make ARCH=s390 CROSS_COMPILE=s390x-linux-gnu- O=kbuild \
         KCFLAGS="-fcommon -fno-pie -fno-stack-protector -fno-PIE -Wno-error" image

- `config-4.0-31bit`: allnoconfig + PRINTK, TN3215(_CONSOLE), BLK_DEV_INITRD,
  INITRAMFS_SOURCE (a tiny `/init`), BINFMT_ELF, EARLY_PRINTK, MARCH_Z900
  (gcc 13 has no g5); `compiler-gcc13.h` added as a copy of `compiler-gcc5.h`.
- `psw_idle-align.patch`: Linux 4.0's 31-bit `psw_idle` builds its wait PSW at
  `__SF_EMPTY(%r15)` = 4(%r15), which is not doubleword aligned; `LPSW`
  takes a specification exception (lx28). The PSW goes to `__SF_EMPTY+4`.
- The reader deck is `arch/s390/boot/image` cut into 80-byte cards (the image
  is already laid out for a VM reader IPL), an `ID MAINT` card in front.

## Kernel #6 and later (12 October 2026): vmcp and vmur

The source is the v4.0 tag (`git clone --depth 1 --branch v4.0
https://github.com/torvalds/linux`), with `compiler-gcc13.h` copied from
`compiler-gcc5.h` and these patches:

| Patch | Why |
|---|---|
| `psw_idle-align.patch` | LPSW needs a doubleword (apply by hand: its hunk has no line numbers) |
| `dma16m-idal.patch`, `entry-mcck-loop.patch` | as before |
| `cputime-dr.patch` | GCC 13 compiles `"d" (CPUTIME_PER_SEC / 2)` (a 64-bit constant) as a register pair and `dr` divides by its high word, zero: a fixed-point divide exception in `init_posix_cpu_timers` at boot. Cast to `unsigned int`. |
| `machine-vm.patch` | Linux knows VM only from STSI 3.2.2, which our CP does not simulate; CPU version X'FF' (STIDP) now also means VM, so `vmcp` and `vmur` load. |
| `vmur-vm370.patch` | VM/370 has neither DIAG X'210' (device class) nor the VM/SP DIAG X'14' subcodes (X'0FFF', X'28'). The device class comes from `QUERY VIRTUAL vdev` (CP command by DIAG 8, into a small kmalloc buffer); the reader is read with CCW READ, one card per I/O, unit exception = end of file. |

`config-4.0-31bit` now has `CONFIG_VMCP=y` and `CONFIG_S390_VMUR=y`.

On Debian:
- `vmcp` issues CP commands, e.g. `vmcp q time` or `vmcp msg cmsuser hello`.
- Before the spool devices can be used, set them online:
  `echo 1 > /sys/bus/ccw/devices/0.0.000c/online`, and the same for `000d`.
- To punch: `vmcp spool 00d to user`, write 80-byte cards to
  `/dev/vmpun-0.0.000d`, then `vmcp close 00d name fn ft`. VM/370's CLOSE
  has no TO option.
- To read: `cat /dev/vmrdr-0.0.000c` reads the reader as 80-byte EBCDIC
  cards, file after file, until it is empty (each file is gone once read,
  as with any VM/370 reader that is not set HOLD). Convert with
  `iconv -f IBM1047`. Needs the CP fix of I-276 (13 Oct): before it, the
  read after the last card waited for ever.
- The s390-tools `vmur` program expects z/VM's response formats, so do not
  use it.

`decks/linux12.rdr` (kernel #12, 15 Oct): kernel #11 (14 Oct) added the LCS driver for the CP guest LAN (9.12b); #12 adds netfilter (conntrack, NAT, filter) and carries Debian 7's iptables 1.4.14 with its libraries in the initramfs, for BORDER (9.12c). `config-4.0-31bit` is its configuration. The initramfs's `/init` also brings up eth0 on an LCS pair (`vmlan=` or 10.3.0.(n+1)) and, with `vmrole=border`, sets up NAT and the `vmfwd=` forwards.

`decks/linux13.rdr` (kernel #13, 16 Oct, 9.40): #12 with `/sbin/vmdns` in the initramfs (`border/vmdns.c`, built by `border/mkvmdns.sh` dynamically against Debian 7's libc, which the initramfs carries for iptables). With `vmrole=border`, `/init` starts it on 10.3.0.1:53 as the LAN's name server: the local zone (`vmdomain=`, default vm370plus.lan; `vmhost=name:address,...`; `vmmx=pref:name,...`) answered by BORDER itself, every other name passed on to `vmdns=` (default the PC). udhcpd hands out the domain, and udhcpc writes it as `search`. BORDER's 191 now holds LINUX13 KERNEL1/KERNEL2.
`decks/linux49.rdr` is a reader deck of the GCC 13-fixed kernel: use it to
boot when the disk's IPL records point to a kernel that does not boot,
then run vmzipl again.
