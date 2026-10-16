# NJE for VM/370plus (9.13b)

VM/370plus is an NJE node through NJE-II (FUNET-NJE,
https://github.com/moshix/NJE-II) in the Debian guest. NJE-II speaks
TCPNJE (port 175), so its peers can be RSCS on z/VM or VM/ESA, JES2 with
TCPNJE, other NJE-II hosts and hobbyist networks.

- **CMS:** `SENDFILE fn ft fm TO user AT node` (CE's own SENDFILE: NETDATA
  to the RSCS machine with a TAG) and `TELL user AT node message`.
  Incoming files arrive in the reader: NETDATA as class N for `RECEIVE`,
  print and punch files as plain cards (class A). Incoming messages arrive as
  `CP MSG` from MAINT (the Debian guest).
- **RSCS** is the gateway machine: `NJEGATE EXEC` (19E) takes each reader file
  and its TAG (`node user`), RECEIVEs it, and hands it to Debian
  (`LXCALL NJE SENDFILE`). AUTOLOG1 starts it. CE's RSCS V1 is `RSCSV1`.
- **Debian:** `/usr/local/nje` (njed, tell, sendfile, qrdr, ucp), the file and
  message exits `njefile`, `njemsg`, `njecards`, and `/etc/init.d/nje`. The
  vmspool service `nje` takes requests from CMS (TELL, SENDFILE, QUERY).
  Configure it with `njeconf /usr/local/nje NODE IP PEER:HOST[:PORT] ...`
  and `/etc/init.d/nje restart`. The node is VM370CE (CMS's SYSTEM NETID)
  and the first peer PCNJE at 10.1.1.1 (an NJE-II on the PC).

The NJE-II source is not in this repository, because it carries the Hebrew
University of Jerusalem's FUNET-NJE licence. `mknje.sh s390|pc [PREFIX]`
fetches a fixed commit from GitHub, patches it for glibc and gcc 10+
(`-fcommon`, `sys_nerr`) and builds it.

On the PC: `sh mknje.sh pc /opt/nje && tar xzf nje-pc.tgz -C /`, copy
`njefile njemsg njecards` to `/opt/nje/lib`, then run
`sh njeconf.sh /opt/nje PCNJE 10.1.1.1 VM370CE:10.1.1.2` and
`/opt/nje/bin/njed`. Files for PC users are kept in `/opt/nje/var/in`.
