# NICOF on VM/370plus (milestone 9.13d)

[NICOF](https://github.com/devhawala/nicof) (Hans-Walter Latz, public
domain) gives CMS:

- a BSD-style socket API (stream and datagram sockets; `NICOFLIB TXTLIB`,
  `SOCKET H`);
- **CMSFTPD**, an FTP server for a CMS user's disks;
- **NHFS** and **HACC**, which copy files between CMS and the PC.

The sockets are the PC's own: NICOF is not a TCP/IP stack. A Java program
on the PC (the *outside proxy*) does the network work. It talks to the
*inside proxy* machines NICOFPXY (PC files) and TCPIPPXY (sockets) over a
3270 line that it DIALs. CMS programs reach the inside proxies through
VMCF.

## What VM/370plus has

| Where | What |
|---|---|
| Directory | `NICOFSRC` (191: the proxies' PROFILE/CHECKCON, 193: the NICOF 0.7.0 binaries), `NICOFPXY`, `TCPIPPXY` (3M, `OPTION ECMODE`), on VM50U1 cylinders 61-70 |
| 19E | `NCFACC EXEC`, which links NICOFSRC 193 and accesses it as N |
| AUTOLOG1 | AUTOLOGs NICOFPXY and TCPIPPXY at IPL. Running disconnected, each runs `RUN$PXY` (`DEFINE GRAF 097`, `IOPROXY`). |
| `vm370ce.conf` | `03CD 3270 NICOFPXY` and `03CE 3270 TCPIPPXY`: the outside proxy connects with the group name as LU name |
| PC: `vm370plus/nicof` | `nicof.jar` (0.7.0), `nicofpxy.properties`, `tcpip.properties`, `nicof.sh start\|stop\|status`, `nhfs/` (the PC files, one directory per CMS user) |

## Use

On the PC, once VM/370 is up (after `AUTOLOG1 DONE - LOGGING OFF`; I-284:
starting it during CP's IPL coincided with a CPU stall and a hung Hercules):

    sh vm370plus/nicof/nicof.sh start      (Java must be on the PATH)

Its log says `Connected to Proxy-VM` once for each proxy.

On CMS:

    NCFACC                         NICOF's disk as N
    NHFS LIST                      the files in vm370plus/nicof/nhfs/<user>
    NHFS GET fn ft (REPLACE        PC -> CMS (text; GETBIN for binary)
    NHFS PUT fn ft                 CMS -> PC
    CMSFTPD -p 2121 -usecmscmds    FTP server on the PC's port 2121, until
                                   the client quits (TERMINATE stops it)

`-usecmscmds` makes CMSFTPD list files with CMS's own commands. That is
safer with our CMS than reading CMS's internal tables. `-pwd x` requires the
password x. With the default, any login is accepted.

## Verified (ncf3, 13 October)

- **The proxies:** both connected on Hercules 3.13.
- **NHFS:** `LIST`, `GET HELLO TEXT` and `PUT PROFILE EXEC` all work.
- **CMSFTPD:**
  - Python's ftplib logged in, listed the disks (A, B, N, S, U, Y) and the
    837 files of A, and stored and read back a text file.
  - A 66,960-byte binary file that contained `FF EF` and `FF FF EF`
    sequences round-tripped unchanged, at about 12 KB/s each way. The
    Hercules 3.x telnet fix that NICOF's documentation asks for was not
    needed for this.

## Notes

- **The inside proxies need `OPTION ECMODE`, or I-283 fixed.** Without it,
  BREXX in a BC-mode machine ended with an operation exception, because
  CP refused STNSM there (I-283, fixed in CP).
- **NICOF 0.7.0's datagram sockets and per-VM IP addresses** (a hosts
  file, `hostbasename`) are not configured.
