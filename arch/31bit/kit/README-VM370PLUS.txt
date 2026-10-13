VM/370plus  --  overlay kit, 13 October 2026 (build 9)
=======================================================

NEW IN BUILD 9
  - THE SPOOL BRIDGE: CMS users call Linux services through reader and
    punch, as with SQLSERV.  Start Debian (cp ipl linux in MAINT, as
    before); vmspoold starts with it.  On the language disk (lang):
        LXCALL service [args]   any service in /etc/vmspool.d of Debian
        PHONE STATUS|CALL ext   Asterisk;  MQTT PUB|SUB topic   mosquitto
        RXPIP RUN fn            REXX packages (rexxpository) for fn REXX
  - DOCUMENTS AND PRINTING (Guide, Chapter 9):
        TROFF fn (MACROS ms [PS   groff        -> fn DOCUMENT / fn PS
        BOOKIE fn [(PS]           BookMaster markup, in the style of the
                                  Guide (try BOOKIE GUIDE (PS)
        LATEX fn [(PDF]           LaTeX (TeX Live 2023) -> fn PS
        PSPRINT fn [(BRIDGE]      PostScript to a printer: by spool (the
                                  00E printer file on the PC, then
                                  vm370plus/tools/psprint.py) or through
                                  Ghostscript in Debian (print.conf)
    Samples on the language disk: GUIDE SCRIPT, SAMPLE SCRIPT, HELLO TEX,
    HELLO TROFF, DEMO REXX.  Ghostscript, groff and LaTeX are also there
    for Linux users.  Source lines are cards: at most 80 columns.
    The bridge and document EXECs are also on MAINT 19D, so they work
    under the usual  ipl cms.  The language disk (samples, compilers) is
    EDF: IPL 290 first (cp def stor 64m, ipl 290, lang); under IPL 190
    LANG ends with DMSACC112S.
    PSPRINT by spool needs the real printer started: on a fresh system
    00E is drained, so an operator enters  start 00e  once (MAINT, or
    vmcp start 00e  in Debian).
    Verified on a fresh CE V1R1.2 with this kit (kt22): LXCALL SYS,
    BOOKIE SAMPLE (PS, LATEX HELLO, PSPRINT SAMPLE -> psprint.py -> PDF.
  - Linux kernel #10: the virtual reader is read to its end (I-276, I-280).
  - Note: a REXX EXEC called with 8 or more arguments ends CMS when the
    GCCLIB segment is loaded -- in the Community Edition too (I-277).

IN BUILD 8
  - THE LANGUAGE DISK (MAINT 19F, volume VMLANG on the new 3380 at 182):
    current compilers and interpreters that run natively on CMS.  After
        ipl 290                     (the EC-mode CMS)
        cp def stor 256m
        lang                        (links 19F and accesses it as L)
    you have:
        CC fn [CPP]     C and C++ (GCC 13.3, binutils 2.42) -> fn MODULE
        COBOL fn        GnuCOBOL 3.1.2;  COBX fn: COBOL with OO
        PLIC fn         PL/I (mikebeer/pli)
        PYTHON fn       Python 3 with NumPy, pandas, SciPy (mikebeer/python)
        SQLITE fn.db    SQLite 3.46;  SQL db stmt via the SQLSERV service
        PCPAS / PCRUN   Stanford Pascal 2026.10
        APL fn.apl      APL (mikebeer/apl)
        XBASE fn.prg    xBase/Clipper (mikebeer/xbase)
        SMALLTLK fn.st  Smalltalk-80 (mikebeer/smalltalk)
    The disk comes in pieces (the last parts of the kit): join them with
    JOINLANG.CMD (Windows) or sh joinlang.sh, like the Debian disk.
    AUTOLOG1 attaches it to the system at every IPL.  The Guide,
    Chapter 8, describes each language.
  - Debian: APL, COBOL (mikebeer/cobol), PL/I (pli, plic), xBase (xbase-c)
    and Smalltalk (stc) in /usr/local/bin, with their tests in
    /usr/local/share/vmtests (sh /usr/local/share/vmtests/runall.sh).
  - The CMS C runtime: /tmp/... names are CMS files, a bare name is
    FILE name (as FILEDEF), SQLite-friendly POSIX file semantics.

IN BUILD 7
  - LARGER MINIDISKS (EDF, 4 KB blocks) under the EC-mode CMS (IPL 290):
        format vaddr fm (blksize 4096
    makes one; a full 3380 (885 cylinders, 518 MB) is one disk, and a
    file may have any number of records (tested: 1,400,000).  See
    LARGER MINIDISKS below.
  - Warm start: SHUTDOWN saves the warm start data, and the next IPL
    warm-starts by itself, spool intact.  No more /cold.
  - TDISK EXEC (MAINT 19D):  tdisk [cyls] [fm]  -- a temporary disk,
    defined, formatted and (with fm) accessed in one step.  HELP TDISK.
  - Linux: each Linux user's IP address follows his CTC pair: pair
    0602/0603 attached as 622/632 is 10.1.2.2 (CMSUSER), 0604/0605
    10.1.3.2, 0606/0607 10.1.4.2; vm370ce.conf has the lines, commented.
    Two Debian systems at once work (guest pages in real frames above
    16 MB).  LINUX READER (IPL from the card reader) works again.
  - CP: an ATTACH of two CTCs next to each other no longer stops CP;
    LPSW/SVC fast paths for PSWs above 16 MB.
IN BUILD 6
  - Debian starts from its own disk:  cp ipl linux  (or LINUX EXEC).
  - The Debian disk is a 3390-9 (7 GB, about 6 GB free).
  - mosquitto (MQTT broker) and Asterisk 1.8 (SIP) under Debian.
  - Linux disk I/O: no 64-CCW limit any more; a memory corruption of
    Linux guests (pages at 16 MB multiples) is fixed.
  - CHATBOT, a service machine (the MCchat persona bot in cREXX), and
    CHAT EXEC for every user (MAINT 19D).  See CHATBOT below.
  - QUERY SET shows ESA ON / PENDING / OFF.
  - RXBVM8 (cREXX on CMS) has TIME() and DATE().
  - The console banner reads VM/370plus.
VM/370 Community Edition V1 R1.2, with CP converted to ESA/390 (AMODE 31)
and an EC-mode CMS on MAINT 290 that can use storage above 16 MB.
Development checkpoint of the VM/370plus project, not a release.
WHAT YOU NEED
  - A fresh, unmodified extraction of VM370CE_V1_R1_2.zip.
    Do NOT apply this kit to a SixPack you care about.
  - Hercules 4.x (SDL Hyperion, tested 4.9.1) or Hercules 3.13.
    Older Hercules 3.0x is untested in ESA/390 mode.
  - A tn3270 emulator (the WC3270 in the CE package is fine).

INSTALL
  1. Unzip VM370CE_V1_R1_2.zip into a new directory.
  2. Unzip ALL kit parts (1-18) OVER it, replacing files:
     part 1: config, system shadows, tools, README; part 2:
     disks/vm50-4.cckd; part 3: the CMSUSER volume's shadow; part 4:
     Linux files (vm370plus/linux/, the Guide) and the VM50-2 shadow
     (MAINT's 19D); part 5: the VM50-3 shadow; parts 6-16: the Debian
     disk in pieces -- then join it: JOINDEB.CMD (Windows) or
     sh joindeb.sh (Linux/WSL), see INSTALL THE DEBIAN DISK; parts
     17-18: the language disk -- join it: JOINLANG.CMD or sh joinlang.sh.
     so the files below replace or add:
        vm370ce.conf                 ARCHMODE ESA/390, CPUMODEL 3090,
                                     ECPSVM NO (CE's own S/370 settings
                                     will not boot this CP)
        disks/vm50-4.cckd            CE's VM50-4 with the paging-area
                                     allocation record fixed (I-239)
        disks/shadows/*_1.shadow     everything else: the ESA/390 CP
                                     nucleus, the EC-mode CMS on 290,
                                     HIGHSTOR, the patched GCCLIB/BREXX,
                                     the directory (ECMODE, 256M), ...
  3. Start it exactly as CE: vm370ce.cmd (Windows) or vm370ce.sh.
  4. CE's hercules.rc IPLs 6A1 by itself when Hercules starts.
     Since build 7 the system warm-starts by itself (the kit carries
     warm start data with an empty spool).  Only if CP asks
        Start ((Warm|Force|COLD|CKPT) (DRain) (DIsable) (NOAUTOlo)) or (SHUTDOWN)
     answer  /cold .
     Once the console shows  DMKCPI966I Initialization complete
     the system is up -- do NOT type ipl 6a1 again (that would reset
     the running system).  Only if nothing has been IPLed (no DMKCPI
     messages at all) type  ipl 6a1  yourself.
  5. Connect the 3270 emulator to localhost:3270.
DISKS
  CMSUSER has 191-195 on VM50U0 and, since build 6, four more 52 MB
  minidisks 196-199 on VM50-8 (cylinders 86-545, which CE leaves free
  for new minidisks); they come formatted (labels CMS196..CMS199):
     access 196 h   (197 i, 198 j, 199 k)
  A CMS minidisk in the classic 800-byte format holds at most about
  52 MB (65,535 blocks of 800 bytes).  Larger ones: see LARGER
  MINIDISKS.

LARGER MINIDISKS  (EDF, build 7)
  Under  ipl 290  CMS also knows IBM's Enhanced Disk Format (4096-byte
  blocks, as VM/SP and z/VM).  The system profile loads the support
  (NUCXMAP shows DMSEFS).  Example with a temporary disk:
        cp define t3350 d10 cyl 20
        format d10 x (blksize 4096
        DMSFOR603R FORMAT WILL ERASE ALL FILES ON DISK 'X(D10)'. ...
        yes
        DMSFOR605R ENTER DISK LABEL:
        edfdsk
        q disk x          ... 3350 4096 ... 2400 blocks
  Without a cylinder count the whole minidisk is used, whatever its
  size (3350 and 3380).  All the usual commands work on it: LISTFILE
  (counts above 99,999 show as e.g. 1400K), TYPE, COPYFILE (also
  APPEND), RENAME, ERASE, MOVEFILE, DISK DUMP, COMPARE, EXECs.
  Limits: only under IPL 290 (the 190 CMS does not know the format);
  a V file can only be extended at its end; TYPE and COPYFILE FROM/FOR
  address records 1-65,535 directly, the rest only in sequence; the
  A disk accessed at IPL must be in the 800-byte format.

USE
  logon cmsuser cmsuser noipl
  cp def stor 128m               HIGHSTOR region = 16 MB .. machine size
  cp link maint 290 290 rr
  ipl 290                        the EC-mode CMS
  highstor query                 HIGHSTOR 16384K- 131072K TOTAL ...
  cp d psw                       030E....  = EC mode

  IPL CMS / IPL 190 still give the classic BC-mode CMS.

  cREXX (the 2022 S370 tree, F0041), built natively on this system:
    ipl 290, then  global txtlib gcclib  and on CMSUSER's E disk:
        rxc -v   rxas -v   rxdas -v   rxbvm -v
        rxbvm -l d ascommon        10 tests, Success
        rxbvm -l d asebcdic        12 tests, Success
    The sources are on CMSUSER's D disk; CRXMAKE EXEC rebuilds them
    (RXVMINTP needs GCC380 in AMODE 31 -- GCC370 runs out of storage).
    Known: RXC -L on a REXX program ends in DLMALLOC PANIC, as on
    stock CE.  vm370plus/ has CRXMAKE.EXEC and the cms.h it needs.

  cREXX cross-compiled on the PC for AMODE 31 (GCC 13 -m31, newlib, the
  CMS runtime in vm370plus/m5f/), on CMSUSER's A disk -- these need
  HERCULES 4.x (they use z900-level instructions 3.13 lacks):
        ipl 290
        rxc31 basic                BASIC REXX -> BASIC RXAS
        rxas31 basic               -> BASIC RXBIN
        rxbvm31 basic              runs it; heap above 16 MB
        libctest a b               C library check (printf, malloc,
                                   files, floating point)

  Compiling C with the compiler running above 16 MB:
    GCC EXEC chooses GCC370 here (no S/380); GCC380 is the AMODE 31
    build.  vm370plus/GCC31.EXEC in this kit forces GCC380 (and
    vm370plus/HELLO31.C is the test program).  Get them onto your
    A disk (e.g. via the card reader: devinit 00c with an
    'ID CMSUSER' card, then READCARD), then
        gcc31 hello31
        global txtlib pdpclib
        load hello31 (start
    (needs  cp def stor 128m  -- GCC380 asks for a 60 MB heap).
LINUX/390 ON VM/370plus  (milestone M7, 9 October 2026)
  A Linux 4.0 kernel (31-bit, ESA/390) runs in a virtual machine.  Kit b4
  adds a complete DEBIAN 7 (wheezy, s390) system on a 3390 disk:
  sysvinit, udev, rsyslog, cron, OpenSSH, apt/dpkg, 154 packages, root on
  ext2, 128 MB swap file.  The kernel is IPLed from the card reader; its
  initramfs (BusyBox) brings the disk and the network up and then hands
  over to Debian (switch_root).  Without the disk you get the BusyBox
  system in memory as before (prompt  vm370plus:~# ).

  Files (vm370plus/linux/):
      linux48.rdr     ID MAINT card + the kernel with its initramfs
      lxparm.rdr      ID MAINT card + the parameter line
                      no_removal_warning conmode=3215 condev=0x0009
      lxparmram.rdr   the same + vmroot=ram: stay in the BusyBox system
                      even if Debian is on the disk (repairs)
  and disks/lnx190.cckd -- the Debian disk, a 3390-9 (build 6; it was a
  3390-1 before), parts 6-16 of the kit, see INSTALL THE DEBIAN DISK.
  INSTALL THE DEBIAN DISK
    The disk is about 290 MB compressed, shipped in eleven pieces (parts
    6-16).  After unzipping them into the CE directory, run there
        JOINDEB.CMD            (Windows cmd.exe)
        sh joindeb.sh          (Linux/WSL; also checks lnx190.sha256)
    which joins disks/lnx190.cckd.001-008 into disks/lnx190.cckd.
    vm370ce.conf in this kit already has the line
        0190    3390    disks/lnx190.cckd
    KEEP A COPY of lnx190.cckd: it is your Linux system disk, and
    Hercules writes to it.

  THE FULL MANUAL is vm370plus/linux/VM370PLUS-Guide.docx (Word): what
  VM/370plus is, the differences to CE, installation, Linux, cREXX.
  START LINUX FROM ITS DISK (build 6): the Debian disk carries its own
  kernel and IPL records.  With 64 MB and the devices attached (step 3
  below, without the reader decks):
        cp def stor 64m
        cp ipl linux                  = SET ESA ON + IPL 250
  From CMS, LINUX EXEC (MAINT 19D, your U disk) does the same:
        linux                         IPL LINUX
        linux reader                  the old way: kernel through the reader
  The kernel parameters (IP address) are on the disk; after changing
  them, run  vmzipl  in Debian (the Guide, Chapter 5).  With  linux
  reader  your IP address comes from  userid LXPARM  on 19D.  Your IP address comes from  userid LXPARM  on 19D
  (MAINT 10.1.1.2, CMSUSER 10.1.2.2), else DEFAULT LXPARM.
  Two Debian systems at once work since build 7 (CP puts guest pages
  in real frames above 16 MB); Debian next to CMS users works.

  BOOT DEBIAN (by hand)
  1. vm370ce.conf has MAINSIZE 256 (CP uses 16 MB as real storage and the
     rest as paging store).  Keep it.  For the network remove the # in
     front of  #0600.2  CTCI ...  (see NETWORK below).
  2. With VM/370plus up (step 4 of INSTALL), at the HERCULES console spool
     the two decks to MAINT's reader, in this order:
        /cp spool 00c class *
        devinit 00c vm370plus/linux/linux48.rdr ebcdic eof
        /cp start 00c
     wait until CP reports the file (RDR FILE ... TO MAINT), then
        devinit 00c vm370plus/linux/lxparm.rdr ebcdic eof
        /cp start 00c
  3. On the 3270:   logon maint cpcms noipl
        cp q rdr all                  two files: the kernel, the parms
        cp def stor 64m               Debian needs the 64 MB
        cp attach 600 to maint as 620 the network (if configured)
        cp attach 601 to maint as 630
        cp attach 190 to maint as 250 the Debian disk
        cp set esa on                 this virtual machine is ESA/390
        cp ipl 00c                    IPL from the reader
     About 4-5 minutes with Hercules: the kernel log, then
        root: the system on /dev/dasda1 -- switch_root
        INIT: version 2.88 booting
        ... Starting OpenBSD Secure Shell server: sshd.
        Debian GNU/Linux 7 vm370plus ttyS0
        vm370plus login:
     Log in as  root , password  vm370plus .
  4. The terminal is a line device: no full-screen programs (use
     nano/vi only if you must; less works as a pager).  CP's MORE...:
     press CLEAR (or PA2).
  5. To leave:  poweroff  (Debian stops its services, then CP shows
     DISABLED WAIT), then  cp logoff  (which releases the adapters and
     the disk).  To run it again, spool the two decks again -- IPL from
     the reader consumes them.
  NETWORK: SSH (PuTTY) INTO LINUX
  Linux talks to the PC over a channel-to-channel adapter that Hercules
  connects to a tun interface on the PC (CTCI).  Linux is 10.1.1.2, the
  PC end is 10.1.1.1.
  a. vm370ce.conf: remove the # in front of
        #0600.2  CTCI    10.1.1.2 10.1.1.1
     Linux PC / WSL2: Hercules needs the tun driver (/dev/net/tun) and
     must run as root (or with hercifc installed setuid).  Windows:
     Hercules needs SDL's CTCI-WIN package (TunTap64.dll, with Npcap)
     next to hercules.exe -- without it the 0600 line fails and the
     rest still works.
  b. Attach the adapters BEFORE  cp ipl 00c  (step 3), each on its own
     virtual control unit (620 and 630, not 620 and 621).
  c. On the PC:  ssh root@10.1.1.2  (PuTTY: host 10.1.1.2, port 22),
     password  vm370plus .  Debian runs OpenSSH (the BusyBox system runs
     Dropbear); the first login may take a minute after boot.
  d. Internet access from Debian needs routing/NAT on the PC;
     /etc/apt/sources.list points at archive.debian.org (wheezy).
  More Linux users (build 7): pair 06xx, attached as 62x/63x, gives
  the address 10.1.n.2 with n = xx/2+1 (0602/0603 -> 10.1.2.2); remove
  the # in front of the matching line (0602.2, 0604.2, 0606.2) in
  vm370ce.conf.  Debian sets the address itself from the pair.
  Other IP addresses: add  vmip=a.b.c.d vmpeer=w.x.y.z  to the line in
  lxparm.rdr, change the 0600 line to match, and in Debian edit
  /etc/network/interfaces.
  Notes:
   - SET ESA ON / OFF switches a virtual machine between S/370 and
     ESA/390; CMS needs it OFF (the default).
   - Linux believes it runs "natively" (no SCLP, no z/VM interfaces),
     so it needs conmode=3215 condev=0x0009.
   - Linux 4.0 is the last kernel with 31-bit support; this one carries
     three fixes (vm370plus/linux/*.patch): 2 KB IDALs for buffers above
     16 MB, the PSW alignment of psw_idle, and an endless loop in the
     31-bit I/O-interrupt return path (entry.S).
   - Works with Hercules 3.13 and 4.x.

CHATBOT  (service machine, build 6)
  The operator starts it:      autolog chatbot chatbot
  Any CMS user asks it:        chat hello there
                               chat /LIST       (personas)
                               chat /npc elena de
                               chat             (a conversation; empty line ends)
  An answer takes a second or two (CHATBOT waits with WAKEUP (RDR).
  Keep your own reader empty while you chat.  To start CHATBOT with the
  system, add  CP AUTOLOG CHATBOT CHATBOT  to AUTOLOG1's PROFILE EXEC.
  Details: the Guide, Chapter 6; sources in vm370plus/chatbot/.
RULES THAT MATTER
  - Always end with  /shutdown  at the Hercules console, then  exit.
    Killing Hercules leaves the shadows inconsistent.
  - Never run two Hercules instances on these disks.
  - Do not IPL something else after IPL CMS (the shared saved system) in
    the same session: leaving a shared system is not finished yet
    (I-235).  Log off and on, or use IPL 190 / IPL 290 (unshared).
  - CP's own machine is 16 MB; with MAINSIZE above 16 (this kit: 256)
    the rest is CP's paging store (milestone M4b).  Virtual machines
    may be up to 256 MB.
  - CPWATCH is no longer autologged (AUTOLOG1's PROFILE EXEC: the two
    CPWATCH lines start with '*').  It is an S/370 CP monitor that loops
    under this CP at priority 5 and starves everyone else (I-251).
  - CP emulates the S/370 interval timer from the TOD clock (ESA/390 has
    none), so time slices end and virtual interval timers run (I-250).
  - CP Q V STOR shows only 5 digits (131072K appears as 31072K) -- a
    cosmetic CP bug (I-246).  DEF STOR shows the right value.
TO GO BACK TO PLAIN CE
  Delete disks/shadows/*_1.shadow, restore vm370ce.conf and
  disks/vm50-4.cckd from the CE zip.
Everything that produced this kit -- the update decks, the generator,
the build tools and the issue log -- is in the project repository.
