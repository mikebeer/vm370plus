# CP/M 2.2 on VM/370plus (milestone 9.33)

CP/M runs under [RunCPM](https://github.com/MockbaTheBorg/RunCPM) (a Z80
emulator by Mockba the Borg, MIT licence) in two places. `mkcpm.sh` builds
both from RunCPM with `runcpm-vm370plus.patch`.

## On CMS: `CPM` (RUNCPM MODULE on the language disk)

Run CPM after `LANG`, under `IPL 290` in a machine of more than 16 MB.

    CPM                              CP/M at the terminal; EXIT ends it
    CPM command [; command ...]      run CP/M commands, then return
    CPM IMPORT fn ft [fm]            a CMS text file becomes a CP/M text
    CPM EXPORT fn ft [fm]            file (ASCII, CR LF, ^Z) in place / back

- **Drives.** CP/M drive X: is the CMS disk X. A: is your A disk. L: is
  the language disk, with the CP/M 2.2 programs PIP, STAT, ASM, LOAD, DDT,
  ED, DUMP, SUBMIT, XSUB, MAC, ZSID and others. A CP/M file NAME.EXT is the
  CMS file NAME EXT, kept as plain bytes. CMS files whose names do not fit
  8.3 are not shown to CP/M.
- **Making files.** CMS cannot hold an empty file. A file that CP/M makes
  therefore exists from its first record on.
- **Console.** The CMS terminal is line-oriented. A typed line goes to CP/M
  character by character, and CP/M's echo of it is suppressed. Console
  status always says "no key", so ^C does not stop a running program.
  Full-screen programs (WordStar and the like) need the Linux route.
- **The CCP.** It is RunCPM's internal CCP (DIR, ERA, TYPE, REN, SAVE,
  USER, and programs as `d:NAME`).

Example (`HELLO ASM` is on L):

    COPYFILE HELLO ASM L = = A
    CPM IMPORT HELLO ASM
    CPM L:ASM HELLO; L:LOAD HELLO; HELLO
    -> Hello from CP/M 2.2 on VM/370plus!

The programs are linked with GCC 13 and cmsrt (`abstraction_cms.h` in the
patch). `IMPORT` and `EXPORT` are in RunCPM's `main.c` under `__CMS__`.

## In the Debian guest

- **`runcpm`** for a Linux user (drive A: is `~/cpm/A/0`). It runs in an
  ssh session, where full-screen programs work.
- **The vmspoold service `cpm`**, which `CPM ... (LINUX` on CMS uses. Each
  CMS user has a CP/M of their own (`/var/lib/cpm/<userid>/A/0`):

      CPM command [; command ...] (LINUX     CPM (FILE fn ft [fm] LINUX
      CPM PUT fn ft [fm] (LINUX              CPM GET fn ft (LINUX
      CPM DIR (LINUX                         CPM RESET (LINUX

  The service runs RunCPM in its batch mode (`-s`, with `RUNCPM_BATCH` set,
  so console status never shows a key and DIR is not cut short).
  `install.sh` installs it.

## The CP/M programs

`A0/` holds the CP/M 2.2 programs and files of RunCPM's distribution disk:
- Digital Research's CP/M 2.2, freely redistributable since the
  Caldera/Lineo licence of 2001;
- RunCPM's own tools;
- classic public-domain utilities.

Programs of uncertain licence (MBASIC, the SLR assemblers, TE, ZEX*,
ZTRAN, CAL) are left out.

## Verified (13 October)

- **Debian (pd25):** `CPM PUT HELLO ASM` and then `CPM ASM HELLO; LOAD
  HELLO; HELLO`, both with `(LINUX`.
- **CMS (cpmlang):** after `LANG`, `CPM DIR L:*.COM; L:STAT A:`,
  `CPM IMPORT HELLO ASM`, and `CPM L:ASM HELLO; L:LOAD HELLO; HELLO`, each
  in a few seconds. `CPM EXPORT` brings the PRN listing back as CMS text.
