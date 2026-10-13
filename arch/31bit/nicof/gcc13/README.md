# NICOF's client for GCC 13 (9.5)

NICOF's socket client (NICOFCLT, NCFSOCKT) built with our GCC 13 (`-m31`,
cmsrt; `arch/31bit/m5f`) instead of CE's GCC and GCCLIB. Programs run under
the EC-mode CMS (`IPL 290`), in AMODE 31, and are ASCII inside.

| file | what |
|---|---|
| `intrapi.h` | the interface NICOFCLT uses, for GCC 13 (replaces CE's INTRAPI H) |
| `ncfintr.s` | the assembler layer: DIAG 68 (VMCF), DIAG 08, WAIT, STIMER in AMODE 24; the external-interrupt handler; the STIMER exit |
| `ncfglue.c` | the C side: HNDEXT, CR0 bit 31, the VMCF queue, ECBs, the timer, EBCDIC for user ids and CP commands |
| `nicofclt.c`, `ncfsockt.c` | NICOF's client, adapted (below) |
| `cmssys.h`, `cmsfile.c` | CE GCCLIB's CMS file functions (CMSfileOpen/Read/Write/Close/State/Erase/Rename) on cmsrt |
| `nhfs.c` ... `udpserv.c` | NICOF's tools, from NICOF 0.6.0 (`ncfbases.c`: LF is ASCII; `nhfscomn.c`: text records translated, ASCII `toupper`) |
| `mkall.sh` | builds every tool into `out/*.text` (`ONLY="NHFS HACC"` for some) |
| `mkhttpd.sh` | builds `HTTPD31.text` alone |
| `nctest.c` | `NCTEST [host [port]]`: gethostbyname, connect, 6000 bytes to an echo server and back, select() with a timeout, close |

Build: `sh mkall.sh` (a few seconds). On CMS (`IPL 290`): `LOAD NHFS (CLEAR`,
`GENMOD NHFS31`, and so on.

| module | from | verified (run i285b) |
|---|---|---|
| NHFS31 | nhfs.c | LIST, PUT, GET, TYPE, PUTBIN, MKDIR, LIST sub; text round trip PC -> CMS -> PC byte for byte, binary PUT byte for byte |
| HACC31 | rnhfs.c | PWD, TYPE, GET, PUT, LIST (needs `service.4 = rawhostfilesvc` in nicofpxy.properties; it reaches every PC file the proxy can, so it is off by default) |
| IPLOOK31 | gethost.c | `-ME`; names are cut to 8 characters by CMS |
| SOCKC31, SOCKS31 | sockc.c, socks.c | client to a PC server and back; server for a PC client |
| TSERVE31 | tserver.c | 3 connections, 20/100/3000-byte echoes, `**TERMINATE**` |
| TSELEC31 | tselect.c | 3 clients at once through select(), `**TERMINATE**` |
| UDPSER31 | udpserv.c | a datagram from the PC echoed |
| NCTEST, HTTPD31 | nctest.c, HTTPD.C | above |
| CMSFTP31 | cmsftpd.c + eeutil.c | Python ftplib: login, NLST of the disks, LIST of A (283 files), RETR text (ASCII lines), STOR text and back, STOR/RETR 5120 binary bytes identical |

Message ids start from the clock (they started at 11): a request an earlier
program left pending at the proxy no longer collides (`VMCF(8) duplicate
message`). The proxy keeps a socket of a program that did not end normally
(EADDRINUSE on the next bind until the proxy is restarted). CMSFTPD needs
Latz's EEUTIL, which is not in NICOF's sources: `eeutil.c` writes the few
functions it uses anew (case helpers, `f_exists`, the pattern check,
DIAG X'00') and GCCLIB's `CMScommand`, `CMSstackQuery` (SENTRIES) and
`CMSconsoleRead` (WAITRD); the disks and files are listed with
`QUERY DISK` and `LISTFILE` (`-usecmscmds` is always on). CMSFTPD's text
records are translated where they meet the disk (readRecord/writeRecord).

## How it differs from CE's INTRAPI

- **No C in the interrupt handler.** DMSITE enters the HNDEXT handler in BC
  mode, AMODE 24 -- no place for GCC 13 code with a stack that may be above
  16 MB. `ncf_exth` takes the interrupt code (old PSW bytes 2-3 for a BC old
  PSW, X'86' for an EC one: I-285), copies each VMCF header (X'4001') into a
  32-slot queue and posts the program's ECB. NICOFCLT takes the queue apart
  in the main line (`drainExt`) before it looks at a request's state.
- **Below 16 MB.** CP's VMCF and CMS's WAIT and STIMER take 24-bit
  addresses, and cmsrt's heap and C stack may be above the line. The request
  buffers come from a static pool (24 requests), and the ECBs and ECB lists
  NICOFCLT and NCFSOCKT wait on are static.
- **ASCII inside.** NICOF's own translation tables become the identity (the
  program's characters are ASCII already); user ids, CP commands and host
  names are translated to EBCDIC at the border, host names from the proxy
  back to ASCII, SMSG text to ASCII.

## Verified (13 Oct, runs i285 and i285b; Hercules 4, CMSUSER 64M, IPL 290)

- HTTPD31: `/`, INDEX.HTML, a 9 KB file byte for byte in 0.6 s, 404, the
  file list of the N disk, `/.stop/key`.
- NCTEST: `localhost is localhost = 127.0.0.1`; 6000 bytes sent and echoed,
  same; select() with nothing to read returns 0 after its timeout (the
  STIMER exit); select after `bye` returns 1, recv 8 bytes.

CMS cuts arguments to 8 characters: `NCTEST localhost` asks for `localhos`.
