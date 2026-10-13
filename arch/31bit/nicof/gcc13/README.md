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
| `mkhttpd.sh` | builds `HTTPD31.text`: `../httpd/HTTPD.C` with `-D__CMSRT__` |
| `nctest.c` | `NCTEST [host [port]]`: gethostbyname, connect, 6000 bytes to an echo server and back, select() with a timeout, close |

Build: `sh mkhttpd.sh`, or
`CFLAGS_EXTRA="-D__CMSRT__ -I. -w" sh ../../m5f/crt.sh NCTEST.text nctest.c nicofclt.c ncfsockt.c ncfglue.c ncfintr.s`.
On CMS (`IPL 290`, NCFACC not needed): `LOAD HTTPD31 (CLEAR`, `GENMOD HTTPD31`.

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
