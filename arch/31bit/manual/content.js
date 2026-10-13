// content.js -- the text of the VM/370plus guide.  Helpers from mkmanual.js.
module.exports = function (M) {
const { P, H1, H1plain, H2, H3, B, N, NOTE, ATTN, SCREEN, TABLE, setAppendix } = M;

// ------------------------------------------------------------------ preface
H1plain('Preface');
P('This manual introduces VM/370plus, tells you how to install it over a VM/370 Community Edition system running under the Hercules emulator, and describes how to use the two things it adds to VM/370: virtual machines larger than 16 MB with programs that run in 31-bit addressing mode, and virtual machines that run the ESA/390 architecture, in which Linux/390 and a complete Debian system run.');
P('It is written for the VM/370 user who already knows CP and CMS: how to log on, how to use the CP commands of the general user (class G) and of the system operator (class A and B), how to edit and run an EXEC. Where VM/370plus behaves as VM/370 does, this manual says nothing; where it differs, it says how and why.');
H3('How this manual is organized');
B(['**Chapter 1, Introduction**, says what VM/370plus is and what it is for.',
   '**Chapter 2, Differences from VM/370 Community Edition**, lists every externally visible change.',
   '**Chapter 3, Installing VM/370plus**, takes you from a fresh Community Edition to a running VM/370plus system.',
   '**Chapter 4, Operating VM/370plus**, covers start-up, shutdown and the rules that keep the system healthy.',
   '**Chapter 5, Running Linux/390**, describes ESA/390 virtual machines, the Linux kernel, the Debian system and the network connection to the PC.',
   '**Chapter 6, Service Machines**, describes CHATBOT, the spool bridge to Linux services (LXCALL, PHONE, MQTT, TELL), RXPIP, and the service machines that are planned.',
   '**Chapter 7, cREXX on VM/370plus**, describes the REXX compiler and virtual machine built for and on VM/370plus, the languages written in cREXX that run on CMS and on Linux, and Turbo CREXX.',
   '**Chapter 8, Compilers on CMS**, describes the language disk and what is on it: GCC 13 for C and C++, GnuCOBOL, COBOL with OO, PL/I, Python, SQLite and SQLSERV, Pascal, APL, xBase, Smalltalk, ZIP and TERSE.',
   '**Chapter 9, Documents and Printing**, describes TROFF, BOOKIE (BookMaster markup), LaTeX and how PostScript reaches a printer.',
   '**Chapter 10, Restrictions and Known Problems**, lists what does not work yet.',
   '**Appendix A** summarizes the new and changed commands, **Appendix B** lists the files in the installation kit, **Appendix C** gives the ESA/390 deviations of the virtual machine, **Appendix D** lists all CP and CMS commands.']);
H3('Conventions');
P('Commands are shown in `monospaced type`. In command descriptions, uppercase letters are typed as shown and lowercase words stand for values you supply. Examples of terminal sessions are shown in framed figures; lines you type are not marked separately because the context makes clear which they are.');
H3('Summary of changes');
P('This edition is kept up to date as VM/370plus grows; each change is listed here, newest first.');
B(['**13 October 2026, night.** NICOF’s tools for the EC-mode CMS, built with GCC 13, on the language disk: `CMSFTP31`, `NHFS31`, `HACC31`, `IPLOOK31`, the web server `HTTPD31` and `NCTEST` (Chapter 6). NICOF programs no longer wait forever under `IPL 290` (I-285), and HX under `IPL 290` no longer ends in a disabled wait (I-286).',
   '**13 October 2026, late evening.** CP/M 2.2 (Chapter 8): `CPM` runs CP/M on CMS, with the CP/M programs on the language disk, and in the Debian guest. TK5 (MVS 3.8j) and a 31-bit MVS are in the plan as M13.',
   '**13 October 2026, evening.** NICOF (Chapter 6): sockets for CMS programs, the FTP server CMSFTPD and NHFS file transfer to the PC. BREXX in a machine without `OPTION ECMODE` no longer ends with an operation exception (I-283).',
   '**13 October 2026, late afternoon.** `TELL user AT NET`: messages to and from users on the internet through the MQTT broker of the Debian guest, with a login per outside user and TLS on the PC (Chapter 6).',
   '**13 October 2026, afternoon.** `TELL` (Chapter 6): messages between CMS users, and between CMS and Linux users (`TELL user AT LINUX`, `tell` on Linux). The bridge and document EXECs are on MAINT\u2019s 19D disk, so they work under `IPL CMS`. A REXX EXEC with eight or more arguments no longer ends CMS in small machines: the GCCLIB segment was saved from an older library and has been saved again (I-277).',
   '**13 October 2026.** The spool bridge (Chapter 6): CMS users call Linux services with `LXCALL`, `PHONE` (Asterisk) and `MQTT`; `RXPIP` brings REXX packages from the rexxpository repository to CMS. Documents and printing (new Chapter 9): `TROFF` (groff), `BOOKIE` (our BookMaster-compatible formatter, which formats this Guide on VM/370plus), `LATEX` (TeX Live 2023), Ghostscript, and `PSPRINT` to a real printer. An ESA/390 guest reading the virtual reader to its end no longer waits forever (I-276).',
   '**12 October 2026, evening (kit build 8).** The language disk VMLANG (MAINT 19F, `LANG`) carries every compiler and tool of Chapter 8; AUTOLOG1 attaches it at IPL. New on CMS: COBOL with OO (`COBX`), xBase and Smalltalk as C programs, the SQLSERV service machine (`SQL`), Stanford Pascal (`PCPAS`, `PCRUN`), APL, `ZIP`, `UNZIP`, `GZIP`, `TERSE` and `UNTERSE`. C programs see real CMS file dates, `/tmp` files and FILEDEF-style names. Debian carries APL, COBOL, PL/I, xBase and Smalltalk with their tests.',
   '**12 October 2026.** Compilers on CMS (new Chapter 8): GCC 13 for C and C++ (`CC`), GnuCOBOL (`COBOL`), PL/I (`PLIC`, also on Debian), Python (`PYTHON`) and SQLite (`SQLITE`); MODULEs run from EDF disks (I-271).',
   '**11 October 2026, evening.** Larger minidisks: CMS under `IPL 290` reads, writes and formats IBM’s Enhanced Disk Format (4 KB blocks): `FORMAT vaddr fm (BLKSIZE 4096`, a full 3380 as one disk, files with more than 65,535 records.',
   '**11 October 2026, late afternoon.** `TDISK` (a temporary disk in one step); Linux IPLs from the reader again (I-265); `LPSW`/SVC fast paths for PSWs above 16 MB.',
   '**11 October 2026, afternoon.** `SHUTDOWN` saves warm start data again and the next IPL warm-starts by itself, spool intact (I-267, I-268); each Linux user’s address follows his CTC pair (9.12a); ATTACH of a CTC next to another no longer stops CP (I-269).',
   '**11 October 2026, late morning.** Two Debian systems at once (M4b.3 complete); ESA/390 access registers saved per virtual machine.',
   '**11 October 2026, morning.** Guest pages in real frames above 16 MB (M4b.3): with `MAINSIZE` above 16, the frames from 16 MB up to 160 MB hold virtual machine pages directly.',
   '**11 October 2026.** GLOBALV.',
   '**10 October 2026, night.** `SET MACHINE` and `QUERY MACHINE`; FULIST (FL); EE, FSVIEW and the REXX full-screen package under the EC-mode CMS; Appendix D, the CP and CMS command tables.',
   '**10 October 2026, evening (kit build 6).** `IPL LINUX` and vmzipl: Debian starts from its own disk; the Debian disk is a 3390-9 (7 GB); no 64-CCW limit for Linux disk I/O; mosquitto (MQTT) and Asterisk under Debian; `QUERY SET` shows ESA; the CHATBOT service machine and `CHAT` (new Chapter 6); RXBVM8 has TIME and DATE; the console banner reads VM/370plus; a fix for memory corruption in Linux guests above 16 MB (location 80).',
   '**10 October 2026.** Regina REXX and THE under Debian; languages written in cREXX for CMS and Linux (BASIC, SNOBOL4, Pascal, Lisp, Smalltalk, PL/M, Prolog, LOGO); Turbo CREXX on both; four more minidisks for CMSUSER (196–199); the native build of the current cREXX (M8.2); Python and GNU C under Debian; milestones brought up to date.',
   '**9 October 2026.** First edition, for kit build 5.']);
P('*PC* means the personal computer on which Hercules runs. *Real* storage and devices are those of the Hercules machine; *virtual* storage and devices are those CP gives a user.');

// ---------------------------------------------------------------- chapter 1
H1('Introduction');
P('VM/370 was written for System/370: 24-bit addresses, 16 MB of storage, S/370 channel I/O. VM/370plus is the VM/370 Community Edition V1 R1.2 with its Control Program (CP) converted to the ESA/390 architecture. It runs on Hercules in `ARCHMODE ESA/390` and gives its users:');
B(['**Virtual machines up to 256 MB.** CP builds ESA/390 segment and page tables, runs in 31-bit addressing mode itself, and gives a virtual machine as much storage as its directory entry allows.',
   '**31-bit programs under CMS.** An EC-mode CMS, IPLed from MAINT’s 290 disk, lets programs obtain storage above 16 MB (the HIGHSTOR facility) and run in 31-bit addressing mode, with a C compiler and library for such programs.',
   '**ESA/390 virtual machines.** `SET ESA ON` makes a virtual machine an ESA/390 machine: channel subsystem I/O (SSCH, TSCH, MSCH, STSCH), the ESA/390 PSW and control registers, home and primary address spaces, IEEE floating point. Linux/390 runs in it.',
   '**Linux and Debian.** A Linux 4.0 kernel (31-bit, the last Linux release that supports 31-bit machines) IPLs from the virtual card reader and runs Debian 7 from a 3390 disk, with SSH access from the PC over a channel-to-channel adapter.',
   '**cREXX.** The cREXX REXX compiler, assembler and virtual machine run on VM/370plus and are built on it.']);
P('Everything else is VM/370: the same CP commands, the same directory, the same CMS for 24-bit work, the same spooling, the same operator. Programs that ran under the Community Edition run unchanged.');
H2('How VM/370plus is built');
P('VM/370plus is a set of UPDATE decks applied to the Community Edition source, assembled on the system itself, and loaded into the CP nucleus with the standard VMFLOAD procedure. The repository of the project holds every deck, the generator that produces them, the build tools, the issue register and the regression runs. Nothing in VM/370plus is a binary patch.');
H2('Milestones');
P('The conversion was done in milestones, each closed by a run that exercises it. Figure 1 shows where they stand.');
TABLE(['Milestone', 'Content', 'Status'], [
  ['M0–M1', 'ESA/390 instructions for the assembler; CP IPLs in ESA/390 mode, console and channel subsystem', 'complete'],
  ['M2', 'ESA/390 DAT tables, CP in 31-bit mode, 31-bit guests', 'complete'],
  ['M3–M4', 'CMS under the new CP, shared CMS by page frame, several users, 4 KB storage keys', 'complete'],
  ['M4b', 'real storage up to 2 GB (above 16 MB as CP\u2019s paging store)', 'complete'],
  ['M4b.3', 'CP uses real frames above 16 MB directly', 'complete: guest pages in frames up to 160 MB; two Debians at once'],
  ['M5', 'EC-mode CMS with storage above 16 MB (HIGHSTOR), 31-bit C (cross and native)', 'complete'],
  ['M6', 'larger minidisks (EDF, 4 KB blocks) under the EC-mode CMS', 'complete'],
  ['M7', 'ESA/390 virtual machines, Linux/390, Debian, SSH', 'complete'],
  ['M7.8', 'Linux for several users from a linkable disk (LINUX EXEC); Linux and CMS side by side', 'complete; two Debians at once since M4b.3'],
  ['M7.9', 'channel programs of any length for ESA/390 guests (no 64-CCW limit)', 'complete'],
  ['M7.10', 'IPL of Linux from its disk (vmzipl) and `IPL LINUX`', 'complete'],
  ['M7.11', 'a 3390-9 Debian disk (about 7 GB) as the standard Linux disk', 'complete'],
  ['\u2014', 'service machines: CHATBOT (the MCchat persona bot) with CHAT EXEC', 'complete'],
  ['M8', 'cREXX: the 2022 release built natively (M5); the current release cross-built (M8.1)', 'complete'],
  ['M8.2', 'the current cREXX built natively with GCC380', 'compiler, assembler and VM work; the library does not build yet'],
  ['\u2014', 'languages in cREXX for CMS and Linux; Turbo CREXX', 'complete on both'],
  ['\u2014', 'GNU C 4.6, Python 2.7 and 3.2, Regina REXX and THE under Debian', 'complete'],
  ['M9', 'before 64-bit: SET/QUERY MACHINE, FULIST, GLOBALV, TDISK; CP leftovers; GCC 13 C/C++, GnuCOBOL, COBOL with OO, PL/I, Python, SQLite + SQLSERV, Pascal, APL, xBase, ZIP, TERSE on CMS; the language disk (kit build 8)', 'complete'],
  ['M9 (next)', 'Guest LAN and the BORDER firewall; CMS sockets and httpd; MQ and MQTT; ooRexx 5; AI gateway; the spool bridge to Linux services (Asterisk, MQTT); RXPIP (REXX packages); NJE; mail; TELL', 'planned'],
  ['M10', 'z/Architecture (64-bit); migration documentation; kit build 9', 'planned'],
  ['M11', 'multiprocessor support; more languages (GNU APL, Java, NetRexx); FreeSWITCH; Node-RED; Docker, QEMU, Bochs; CFSERV; kit build 10', 'planned'],
], [1400, 6038, 2200], 'VM/370plus milestones');

// ---------------------------------------------------------------- chapter 2
H1('Differences from VM/370 Community Edition');
H2('The machine');
TABLE(['Item', 'Community Edition', 'VM/370plus'], [
  ['Hercules architecture', '`ARCHMODE S/370`', '`ARCHMODE ESA/390`'],
  ['CPU model', '4381, ECPS:VM available', '3090, ECPS:VM not used (its assists are S/370-only)'],
  ['Hercules main storage', '16 MB', '64 MB in the kit; up to 2048 MB'],
  ['CP’s addressing mode', '24-bit', '31-bit'],
  ['Real storage used by CP', 'up to 16 MB', '16 MB as frames; the rest as a fast paging store'],
  ['I/O', 'S/370 channels (SIO, TIO, HIO)', 'ESA/390 channel subsystem (SSCH, TSCH, HSCH)'],
  ['Storage keys', '2 KB keys', '4 KB keys (ISKE, SSKE, RRBE)'],
  ['Segment size', '64 KB', '1 MB (ESA/390 has no other)'],
], [2400, 3200, 4038], 'The real machine');
H2('Virtual machines');
B(['**Storage.** A virtual machine may have up to 256 MB. The directory’s maximum (the second size on the USER statement) is checked as before; the kit’s directory allows 256 MB for CMSUSER and 64 MB for MAINT.',
   '**Shared CMS.** The CMS saved system is shared by page frame rather than by segment: with 1 MB segments, sharing whole segments would expose private CMS storage to other users. The effect for the user is none.',
   '**31-bit virtual machines.** A virtual machine in EC mode may run with PSW bit 32 on (31-bit addressing); CP translates its addresses in 31 bits.',
   '**ESA/390 virtual machines.** New command `SET ESA ON|OFF`; see Chapter 5 and Appendix A.',
   '**More disk space for CMSUSER.** Besides 191–195 on VM50U0, CMSUSER has four more minidisks, 196–199, of 115 cylinders (about 52 MB) each on VM50-8, in the cylinders the Community Edition leaves free there. They come formatted, labeled CMS196 to CMS199: `ACCESS 196 H` (197 I, 198 J, 199 K). A CMS minidisk on a 3350 holds at most 65,535 blocks of 800 bytes, about 52 MB; larger minidisks come with milestone M6.']);
H2('CP commands');
TABLE(['Command', 'Change'], [
  ['`SET ESA ON|OFF`', 'New (class G). ON makes the next IPL of a device an IPL of an ESA/390 machine; OFF returns to System/370 at once.'],
  ['`SET MACHINE 370|XA|ESA`, `QUERY MACHINE`', 'New (class G), the z/VM names. SET MACHINE ESA (or XA) is SET ESA ON, SET MACHINE 370 is SET ESA OFF.'],
  ['`IPL cuu`', 'A user who is running a shared named system (CMS) is IPLed with CLEAR, so that no copy of the shared pages stays in the virtual machine.'],
  ['`QUERY SET`', 'A fifth line shows `ESA ON`, `ESA PENDING` (SET ESA ON before the next IPL of a device) or `ESA OFF`.'],
  ['`IPL LINUX`', 'New. IPLs the Debian disk at virtual address 250 as an ESA/390 machine: `SET ESA ON` and `IPL 250` in one command. Works from a directory IPL statement too.'],
  ['`IPL name`', 'Any other name always IPLs a System/370 machine; a pending `SET ESA ON` waits for the next IPL of a device.'],
  ['`DEFINE STORAGE`', 'Up to 256 MB. Leaves the named system the user was running.'],
  ['`DISPLAY`, `STORE`', 'Address storage above 16 MB.'],
  ['`QUERY VIRTUAL STORAGE`', 'Shows five digits only (131072K appears as 31072K); `DEFINE STORAGE` echoes the right value.'],
], [3000, 6638], 'Changed CP commands');
H2('CMS');
B(['The ordinary CMS (`IPL CMS`, `IPL 190`) is unchanged: a System/370 BC-mode CMS with 16 MB of addressable storage.',
   'An EC-mode CMS is on MAINT’s 290 disk (`IPL 290`). With it, the HIGHSTOR nucleus extension (loaded by the system profile) manages storage from 16 MB to the end of the virtual machine; SVC 120 (GETMAIN/FREEMAIN RU) gives programs storage above the line.',
   'GCC380, the AMODE 31 build of the Community Edition’s GCC, runs above 16 MB; GCCLIB31 is a C library for programs that run in 31-bit mode with their heap above 16 MB.',
   'Under the EC-mode CMS (`IPL 290`), minidisks can use IBM’s Enhanced Disk Format (EDF, 4 KB blocks) besides the 800-byte format: a disk up to the size of a full 3380 (518 MB), files with any number of records. `FORMAT vaddr fm (BLKSIZE 4096` makes one; ACCESS tells the formats apart by the label. The DMSEFS nucleus extension does the work and is loaded by the system profile.',
   'New commands on MAINT 19D: `FULIST` (`FL`), a full-screen file list in the manner of FILELIST; `WAKEUP`; `CHAT`; `LINUX`. `GLOBALV`, global variables as in z/VM, is loaded at IPL. Appendix A; Appendix D lists all CP and CMS commands.',
   'Programs written for System/370 BC mode (MECAFF’s EE and FSVIEW, the REXX full-screen package) run under the EC-mode CMS too: CMS turns an SVC from BC mode into an EC call, and CP takes a BC-style system mask (`SSM =X\'FF\'`) in an EC-mode System/370 machine as the BC mask it is.',
   'CPWATCH is not autologged: it is an S/370 monitor that loops under this CP. The two lines in AUTOLOG1’s PROFILE EXEC are commented out.']);

// ---------------------------------------------------------------- chapter 3
H1('Installing VM/370plus');
H2('What you need');
B(['A fresh, unmodified extraction of `VM370CE_V1_R1_2.zip`. Do not install VM/370plus over a system you care about.',
   'Hercules 4.x (SDL Hyperion; tested with 4.7 and 4.9.1) or Hercules 3.13. Programs cross-compiled for 31-bit mode need Hercules 4.x, which has the z900-level instructions in ESA/390 mode.',
   'A 3270 terminal emulator; the wc3270 in the Community Edition package is sufficient.',
   'For Linux networking on Windows: the CTCI-WIN package for SDL Hercules (TunTap64.dll, with Npcap). On Linux or WSL2: the tun driver, and Hercules running as root or with hercifc installed setuid.',
   'The VM/370plus kit: twelve zip files, each under 30 MB.']);
TABLE(['Part', 'Contents'], [
  ['1', '`vm370ce.conf` (ESA/390), the shadow files of the system volumes (CP nucleus, CMS, directory), README, tools'],
  ['2', '`disks/vm50-4.cckd`: VM50-4 with its paging area record corrected'],
  ['3', 'the shadow file of the CMSUSER volume'],
  ['4', '`vm370plus/linux/`: the Linux kernel and parameter decks, the CMS files for MAINT 19D, kernel configuration and patches'],
  ['5', 'the shadow file of VM50-3 (spool and paging)'],
  ['6–12', 'the Debian disk, `disks/lnx190.cckd` (a 3390-9), in seven pieces, with `JOINDEB.CMD` and `joindeb.sh` to join them'],
], [1000, 8638], 'The installation kit');
H2('Installation steps');
N(['Unzip `VM370CE_V1_R1_2.zip` into a new directory.',
   'Unzip kit parts 1 to 15 over it, replacing files when asked.',
   'Join the Debian disk. In the Community Edition directory, at a Windows command prompt:',
]);
SCREEN(['JOINDEB.CMD', 'Debian disk joined: disks\\lnx190.cckd'], 'Joining the Debian disk (Windows)');
P('On Linux or WSL2 use `sh joindeb.sh`, which also checks the result. The file `disks/lnx190.sha256` holds the checksum of the joined file. Keep a copy of `lnx190.cckd`: it is your Linux system disk and Hercules writes to it.');
P('Join the language disk the same way (kit parts 14 and 15): `JOINLANG.CMD` on Windows or `sh joinlang.sh` on Linux and WSL2 gives `disks/lang.cckd`, the 3380 at address 182 that holds MAINT 19F (see Chapter 8).');
SCREEN(['JOINLANG.CMD', 'Language disk joined: disks\\lang.cckd'], 'Joining the language disk (Windows)');
N(['If you want the Linux network, remove the `#` in front of the line `#0600.2 CTCI 10.1.1.2 10.1.1.1` in `vm370ce.conf`.',
   'Start the system as you start the Community Edition: `vm370ce.cmd` on Windows, `vm370ce.sh` elsewhere.'], true);
H2('The first IPL');
P('The Community Edition’s `hercules.rc` IPLs from 6A1 by itself when Hercules starts. The first time, CP has no warm-start data and asks for the start type; answer `/cold`. When the console shows `DMKCPI966I Initialization complete`, the system is up.');
SCREEN(['HHCCP041I SYSCONS interface active',
        'VM/370plus Online',
        'Start ((Warm|Force|COLD|CKPT) (DRain) (DIsable) (NOAUTOlo)) or (SHUTDOWN)',
        '/cold',
        'NOW 09:51:57 GMT FRIDAY 10/09/26',
        'DMKCPI966I Initialization complete'], 'The first IPL of VM/370plus');
ATTN('Do not type `ipl 6a1` again once CP has initialized: it resets the running system. Type it yourself only if nothing has been IPLed (no DMKCPI messages at all).');
P('Then connect the 3270 emulator to `localhost:3270` and log on as usual.');
H2('Going back to the Community Edition');
P('Delete `disks/shadows/*_1.shadow` and restore `vm370ce.conf` and `disks/vm50-4.cckd` from the Community Edition zip.');

// ---------------------------------------------------------------- chapter 4
H1('Operating VM/370plus');
H2('Start-up and shutdown');
P('Operation is that of VM/370. The operator’s console is 0009; `ENABLE ALL` brings up the 3270 terminals. To stop the system, the operator types `shutdown` on 0009 and then `exit` at the Hercules console.');
ATTN('Always end with `shutdown` at the operator’s console before you leave Hercules. Killing Hercules leaves the shadow files inconsistent. Never run two Hercules instances on the same disks.');
H2('Storage');
P('CP’s own machine is 16 MB: its nucleus, free storage, locked and V=R pages, spool buffers and the pages of named systems stay below 16 MB. With `MAINSIZE` above 16 in `vm370ce.conf` (the kit has 64), the frames from 16 MB up to 160 MB hold virtual machine pages directly, and the storage above 160 MB, if any, is a paging store that no channel addresses: a page-out copies a frame there, a page-in copies it back. Up to 2048 MB has been tested. Virtual machines may be up to 256 MB.');
NOTE('Because CP keeps its working frames in the low 16 MB, a 64 MB Linux machine pages heavily, and while one runs, other users’ work is slower. Using frames above 16 MB directly (milestone M4b.3) is planned.');
H2('Rules that matter');
B(['A user who has IPLed the shared CMS may IPL another system; the shared pages are dropped first (the IPL is done with CLEAR). The shared-system models stay in storage until CP is IPLed again.',
   'CP emulates the System/370 interval timer from the TOD clock, since ESA/390 has none; time slices end and virtual interval timers run.',
   'CPWATCH is not autologged (see Chapter 2).']);

// ---------------------------------------------------------------- chapter 5
H1('Running Linux/390');
P('This chapter describes ESA/390 virtual machines and Linux/390 in them. Linux runs in two forms: a small system in storage (BusyBox), which needs no disk, and Debian 7 on a 3390 disk of its own.');
H2('ESA/390 virtual machines');
P('`SET ESA ON` makes a virtual machine an ESA/390 machine at its next IPL of a device. Such a machine has:');
B(['the ESA/390 PSW and control registers, primary, secondary and home address spaces, and ESA/390 DAT, which CP shadows;',
   'the channel subsystem: its devices are subchannels, I/O is started with SSCH and completed with TSCH; channel programs may use format-0 or format-1 CCWs and 31-bit IDAWs;',
   'IEEE binary floating point, the CPU timer, the clock comparator and SIGP for its one processor.']);
P('`IPL CMS` (or any IPL by name) gives a System/370 machine again; the pending `SET ESA ON` then applies to the next IPL of a device. `SET ESA OFF` returns to System/370 at once. Appendix C lists where the virtual machine deviates from the ESA/390 architecture.');
H2('What a Linux user needs');
TABLE(['Resource', 'Virtual address', 'How it is provided'], [
  ['Storage', '64 MB', '`DEFINE STORAGE 64M`, or 64M in the directory'],
  ['Console', '009 (3215)', 'the terminal; Linux uses it in line mode'],
  ['Card reader', '00C', 'the kernel and the parameters are IPLed from it'],
  ['Debian disk', '250 (3390)', 'a real 3390, attached or dedicated to the user'],
  ['Network', '620 and 630 (CTCA)', 'a CTC pair, attached or dedicated to the user'],
], [2000, 2400, 5238], 'Resources of a Linux virtual machine');
P('The operator attaches the devices:');
SCREEN(['attach 600 to maint as 620', 'attach 601 to maint as 630', 'attach 190 to maint as 250'], 'Attaching a Linux user’s devices (operator)');
P('Each device must be on its own virtual control unit (620 and 630, not 620 and 621). The second user’s pair 602/603 goes to 622 and 632, which gives that machine 10.1.2.2. For a permanent arrangement put `DEDICATE` statements in the user’s directory entry.');
H2('Starting Linux from its disk: IPL LINUX');
P('The Debian disk carries its own kernel and IPL records, so Linux IPLs straight from it. With 64 MB of storage and the devices attached (the disk at 250), one CP command is enough, from CMS or from CP:');
SCREEN(['logon cmsuser',
        'cp define storage 64m',
        'cp ipl linux',
        'Linux version 4.0.0+ ...',
        '...',
        'Debian GNU/Linux 7 vm370plus ttyS0',
        'vm370plus login:'], 'Starting Linux from its disk');
P('`IPL LINUX` is `SET ESA ON` followed by `IPL 250`. Options of the device IPL may follow (`IPL LINUX CLEAR`). The kernel parameters, including the IP address, were written to the disk with the kernel by `vmzipl` (below); the reader route that follows remains for a different kernel or other parameters.');
H2('Making a disk IPLable: vmzipl');
P('`vmzipl` is the VM/370plus counterpart of `zipl`, for the 31-bit Linux 4.0 kernel. It is installed on the Debian disk as `/usr/local/sbin/vmzipl`. Run it as root after changing the kernel or its parameters:');
SCREEN(['vmzipl',
        'vmzipl: /dev/dasda: kernel 1775 blocks in 151 runs, channel program 2560 CCWs in 5 blocks',
        'vmzipl: parameters: no_removal_warning conmode=3215 condev=0x0009 vmip=10.1.1.2 vmpeer=10.1.1.1',
        'vmzipl: IPL with  CP SET ESA ON  and  CP IPL <address of this disk>'], 'Running vmzipl');
TABLE(['Option', 'Meaning'], [
  ['`-k file`', 'the kernel image (default `/boot/linux48.image`)'],
  ['`-p "parameters"`', 'the kernel parameter line (default: the running system\u2019s `/proc/cmdline`)'],
  ['`-d device`', 'the disk to make IPLable (default `/dev/dasda`)'],
  ['`-r directory`', 'where that disk\u2019s file system is mounted, when it is not the root'],
], [2400, 7238], 'vmzipl options');
P('It writes `/boot/vmipl.bin` (the kernel with the parameters) and `/boot/vmipl.ccw` (the channel program that reads it), then records 1 and 2 of track 0 (IPL1 and IPL2) of the disk.');
H2('Starting Linux with the LINUX EXEC');
P('The `LINUX EXEC` is on MAINT’s 19D disk, which every CMS user accesses as the U disk. `LINUX` alone issues `CP IPL LINUX`. `LINUX READER` takes the older route: the kernel and the IPL parameter files on 19D go through your reader. From CMS, with 64 MB of storage and the devices attached:');
SCREEN(['logon cmsuser',
        'cp define storage 64m',
        'ipl cms',
        'Ready;',
        'linux reader',
        'LINUX: punching the kernel and CMSUSER LXPARM to your reader ...',
        'PUN FILE 0002  TO  CMSUSER  COPY 01 NOHOLD',
        'PUN FILE 0003  TO  CMSUSER  COPY 01 NOHOLD',
        'LINUX: IPL 00C as an ESA/390 machine',
        'Linux version 4.0.0+ ...',
        '...',
        'root: the system on /dev/dasda1 -- switch_root',
        'INIT: version 2.88 booting',
        '...',
        'Debian GNU/Linux 7 vm370plus ttyS0',
        'vm370plus login:'], 'Starting Linux from CMS');
P('The EXEC holds the files already in your reader, punches the two pieces of the kernel into your own reader as one spool file and then your parameter file, sets `ESA ON` and IPLs the reader. The parameter file is `userid LXPARM` on the U disk, or `DEFAULT LXPARM` if there is none; it carries your IP address. Afterwards `CP CHANGE RDR ALL NOHOLD` releases your other reader files.');
TABLE(['File on MAINT 19D', 'Contents'], [
  ['`LINUX48 KERNEL1`, `KERNEL2`', 'the kernel with its initramfs, 91,696 card images in two files (a CMS file holds at most 65,533 records)'],
  ['`LINUX EXEC`', 'the EXEC described above'],
  ['`DEFAULT LXPARM`', '`no_removal_warning conmode=3215 condev=0x0009`'],
  ['`userid LXPARM`', 'the same plus `vmip=` and `vmpeer=`, the user’s address and the PC’s end'],
], [3000, 6638], 'Linux files on MAINT 19D');
H2('Starting Linux by hand');
P('Without the EXEC, the operator spools the two decks from `vm370plus/linux/` to the user’s reader, and the user IPLs it:');
SCREEN(['/cp spool 00c class *',
        'devinit 00c vm370plus/linux/linux48.rdr ebcdic eof',
        '/cp start 00c',
        'devinit 00c vm370plus/linux/lxparm.rdr ebcdic eof',
        '/cp start 00c'], 'Spooling the Linux decks (Hercules console)');
SCREEN(['logon maint cpcms noipl', 'cp define storage 64m', 'cp set esa on', 'cp ipl 00c'], 'IPL of Linux from the reader');
P('The ID card of the two decks names MAINT; for another user change the first card or spool the decks with `/cp spool 00c ...` and `TRANSFER`. IPL from the reader consumes the files: spool them again for the next IPL.');
H2('The Linux system');
H3('Boot sequence');
P('The kernel’s initramfs (BusyBox) brings the 3390 online, creates a partition and a file system on a new disk, groups the CTC pair as `ctc0` and configures it from the IPL parameters (default 10.1.1.2, peer 10.1.1.1). If the disk holds a system (`/sbin/init`), it hands over to it with `switch_root`; otherwise it stays in storage with the prompt `vm370plus:~#`. The parameter `vmroot=ram` (deck `lxparmram.rdr`) keeps the in-storage system even when Debian is on the disk, which is how you repair the disk.');
H3('Debian');
P('The disk is a 3390 model 9 (10,017 cylinders, about 7 GB). It holds Debian 7.11 (wheezy) for s390: sysvinit, udev, rsyslog, cron, OpenSSH, apt and dpkg, root on ext2 (6.8 GB, about 6 GB free), a 128 MB swap file. The disk carries its own kernel and IPL records, so `IPL LINUX` starts it. Log in as `root` with password `vm370plus`, and change the password.');
P('Development tools are installed: GNU C and C++ 4.6.3 with make, Python 2.7.3 (`python`) and Python 3.2.3 (`python3`), Regina REXX 3.6 (`regina` or `rexx`, classic REXX as on CMS) and THE 3.3, The Hessling Editor (`the`, an XEDIT-like editor; use it over SSH, since the 3215 console cannot show a full screen). cREXX and the languages of Chapter 7 are in `/usr/local/bin`.');
P('Servers are installed too. **mosquitto 1.6.15**, an MQTT broker (`/usr/local/sbin/mosquitto`, configuration `/etc/mosquitto/mosquitto.conf`, port 1883), with the clients `mosquitto_pub` and `mosquitto_sub`; start it with `mosquitto -d -c /etc/mosquitto/mosquitto.conf`. **Asterisk 1.8.13**, the telephone exchange, starts at boot and listens for SIP on UDP port 5060; `asterisk -rx "core show uptime"` talks to it. Both are offered as experiments: they work, at the speed of an emulated mainframe.');
SCREEN(['root@vm370plus:~# mosquitto -d -c /etc/mosquitto/mosquitto.conf',
        'root@vm370plus:~# mosquitto_sub -t vm370plus/test -C 1 &',
        'root@vm370plus:~# mosquitto_pub -t vm370plus/test -m "hello MQTT on VM/370plus"',
        'hello MQTT on VM/370plus'], 'MQTT on Debian');
SCREEN(['root@vm370plus:~# cat /etc/debian_version', '7.11',
        'root@vm370plus:~# df -h /', 'Filesystem      Size  Used Avail Use% Mounted on', '/dev/dasda1     6.8G  541M  5.9G   9% /',
        'root@vm370plus:~# free', '             total       used       free     shared    buffers     cached',
        'Mem:         60512      21688      38824          0       2220      13792'], 'Debian on VM/370plus');
H3('Languages on Debian');
P('Besides GNU C, Python and Regina, the C89 interpreters and translators of the VM/370plus languages are in `/usr/local/bin`: `apl` (APL), `cobol` (COBOL with OO, mikebeer/cobol: `cobol prog.cob -o prog.c`, then `gcc -I/usr/local/share/cobol`), `pli` and `plic` (PL/I: `plic prog.pli` translates and compiles), `xbase-c` (xBase) and `stc` (Smalltalk: `stc -L /usr/local/share/vmtests/smalltalk/lib prog.st`). `sh /usr/local/share/vmtests/runall.sh` runs their test suites (APL 34, COBOL 20, PL/I 39, xBase 7, Smalltalk 22 tests).');
H3('The console');
P('The 3215 is a line device. Programs that use the full screen (vi, top without `-n1`) draw poorly; use them only if you must. Ctrl-C cannot be typed: type `^c` and Enter instead (the 3215 driver turns `^c`, `^d` and `^z` into the control characters). When CP shows MORE..., press CLEAR or PA2.');
H3('Network and SSH');
P('Linux talks to the PC over a channel-to-channel adapter that Hercules connects to a tun interface on the PC (CTCI). From the PC: `ssh root@10.1.1.2`, or PuTTY to host 10.1.1.2, port 22. The first login after a boot may take a minute while the SSH server gathers randomness. Internet access from Linux needs routing and NAT on the PC; `/etc/apt/sources.list` points at archive.debian.org.');
H3('Stopping Linux');
P('`poweroff` (or `halt`) stops Debian; CP then shows a disabled wait. `CP LOGOFF` releases the attached devices. `IPL CMS` returns to CMS in the same session.');
H2('Several Linux users');
P('Each Linux user needs his own copy of the Debian disk and his own CTC pair with a CTCI line in `vm370ce.conf`. The address follows the pair: real pair 06xx, attached as 62x and 63x (x = xx: 600/601 as 620/630, 602/603 as 622/632) or at its real address, gives the guest 10.1.n.2 with n = xx/2 + 1, and the CTCI line’s PC end is 10.1.n.1. A boot script on the Debian disk (`vm370plus-net`) sets it, so one disk image serves every user. The kit’s example: MAINT on 600/601 and 190, address 10.1.1.2; CMSUSER on 602/603 and 191, address 10.1.2.2.');
SCREEN(['0600.2  CTCI    10.1.1.2 10.1.1.1', '0602.2  CTCI    10.1.2.2 10.1.2.1', '0190    3390    disks/lnx190.cckd', '0191    3390    disks/lnx191.cckd'], 'Two Linux users in vm370ce.conf');
P('Make `lnx191.cckd` a copy of the Debian disk. The disks are identical: the address comes from the IPL parameters, not from the disk. Give Hercules enough storage for CP\u2019s paging store: `MAINSIZE 256` (the kit\u2019s value) for two Linux users, more for more. With too little, CP fills its DASD paging space (`DMKPGT400I SYSTEM TEMP SPACE FULL`) and the Linux machines fail.');
P('Two Debian systems run at the same time: CP gives virtual machine pages frames above 16 MB (with `MAINSIZE 256`, up to 160 MB), so both working sets stay in storage. With `CP IPL LINUX` from the disk, each system takes the IP address in the disk\u2019s own parameters; give the second one its address with `ip addr` (or IPL it from the reader with LINUX EXEC and its LXPARM).');
H2('Linux and CMS side by side');
P('Linux users and CMS users share the system as any two VM/370 users do. Tested: one user runs Debian and writes and reads files under load, while another user works in CMS (LISTFILE, COPYFILE, TYPE, ERASE), defines 64 MB, starts Linux with `LINUX`, stops it with `poweroff` and goes back to CMS with `IPL CMS`; the Debian system\u2019s files are unchanged throughout.');

// ---------------------------------------------------------------- chapter 6
H1('Service Machines');
P('A service machine is a virtual machine that works for other users: it runs disconnected, takes requests and sends answers. VM/370 has always had them (the operator, RSCS); VM/370plus adds new ones. The first is CHATBOT. Others are planned (see the end of this chapter).');
H2('CHATBOT');
P('CHATBOT is the persona chatbot of MCchat, written by the project owner in cREXX and run by RXBVM8. It has ten personas, each in English and German, and a knowledge base, all as CMS files; every answer is authored text chosen the way the original bot chooses it. There is no language model and no network.');
P('Users talk to it through the spool. `CHAT` punches the question to CHATBOT’s reader. CHATBOT reads its reader, runs the bot once for all waiting questions, and punches each answer to the reader of the user who sent the question (the spool file’s origin, so nobody can read another user’s answers). The bot keeps each user’s conversation (persona, language, topic) in `userid CBSESS` on its 191 disk.');
SCREEN(['chat hello there',
        'Hello.',
        'I’m Tchaika.',
        'So - what brings you here today?',
        'Ready;',
        'chat /LIST',
        'Personas (name/language): amicus/de amicus/en andron/de andron/en ...',
        'chat /npc elena de',
        'Oh - du bist es.',
        'chat wie geht es dir?',
        'Gut genug, um dich dasselbe zu fragen und es ernst zu meinen. Also - wie geht es dir wirklich?',
        'chat /WHO',
        'You are CMSUSER. You are talking to elena (de).'], 'Talking to CHATBOT from CMS');
P('An answer takes a second or two: CHATBOT waits with `WAKEUP (RDR`, which returns as soon as a file arrives in its reader, and looks for a stop request every 30 seconds (INTERVAL in CHATBOT CONFIG). `CHAT` alone starts a conversation that ends with an empty line.');
TABLE(['Command', 'Meaning'], [
  ['`/HELP`', 'the commands'],
  ['`/LIST`', 'the personas and languages installed'],
  ['`/NPC name [en|de]`', 'talk to another persona'],
  ['`/LANG en|de`', 'change the language'],
  ['`/RESET`', 'start the conversation again'],
  ['`/WHO`', 'who you are and whom you talk to'],
  ['`/DEBUG ON|OFF`', 'show how the bot chose its answer'],
], [2600, 7038], 'CHATBOT commands');
H2('Running CHATBOT');
P('The directory entry is in place: 32 MB, `OPTION ECMODE`, `IPL 290`, a 60-cylinder 191 on VM50U1 that holds the bot, its data and the sessions. Start it from the operator console:');
SCREEN(['autolog chatbot chatbot', 'AUTO LOGON   ***   CHATBOT  USERS = 005'], 'Starting CHATBOT');
P('Its PROFILE EXEC starts the bot when the machine is disconnected, as after AUTOLOG. An interactive logon (`logon chatbot`) gets CMS instead, for maintenance: `EXEC CHATBOT (LOUD` then runs the bot on the console, `EXEC CHATBOT (ONCE LOUD` answers what is waiting and stops. To stop the service, create `CHATBOT STOP A` on its 191 (the loop ends at its next look) or `FORCE CHATBOT`. To start it with the system, add `AUTOLOG CHATBOT CHATBOT` to the start-up (AUTOLOG1’s PROFILE EXEC).');
TABLE(['CHATBOT CONFIG key', 'Meaning'], [
  ['`DEFNPC`, `DEFLANG`', 'persona and language for a new user (tchaika, en)'],
  ['`WIDTH`', 'line width of the answers (72)'],
  ['`CRISIS`', 'YES: words from a persona’s crisis list get its care text'],
  ['`BOTCMD`', 'the command that runs the bot: `RXBVM8 CHATBOT -a CMS`'],
  ['`MAXBATCH`, `INTERVAL`', 'questions per run of the bot (20), seconds between looks at the reader (30)'],
  ['`VAR name value`', 'variables the persona scripts may use'],
], [3000, 6638], 'CHATBOT CONFIG');
NOTE('The upstream bot (build 1) runs unchanged. Its EXECs were adapted to VM/370 and BREXX (no WAKEUP, no UPPER, EXECIO with variable records, AUTOLOG instead of XAUTOLOG) and one bug was fixed; the list is in `vm370plus/chatbot/VM370PLUS.TXT`.');
H2('SQLSERV');
P('SQLSERV, the SQLite service machine, is in service: see Chapter 8, SQLite. It is the model for the service machines below: a request is a file punched to the machine, the answer is punched back to the sender.');
H2('The spool bridge: LXCALL');
P('The spool bridge gives CMS users the services of the Debian guest through the same protocol as SQLSERV and CHATBOT. `vmspoold`, a daemon in the Debian guest, reads its virtual reader, runs the requested service and punches the answer back to the sender. The guest is MAINT unless `GLOBALV SELECT LXCALL SET SERVER userid` names another.');
P('`LXCALL service [arguments]` sends a request and shows the answer, which is also kept as `LXCALL RESULT A` (its first line is `VMSPOOL RC n`); the return code of LXCALL is that of the service, 404 if there is no such service. Options: `(FILE fn ft fm` sends a file as the input of the service, `(SERVER userid`, `(WAIT n` (seconds, default 900), `(NOTYPE`. A service is any executable in `/etc/vmspool.d` of the guest; `echo` and `sys` are there to try the bridge.');
SCREEN(['lxcall sys', 'Linux vm370plus 4.0.0+ #8 Tue Oct 13 00:11:13 UTC 2026 s390 GNU/Linux', ' 01:26:49 up 10 min,  1 user,  load average: 0.93, 1.75, 1.15', 'Ready;'], 'A Linux service called from CMS');
H2('PHONE and MQTT');
P('`PHONE STATUS`, `PHONE CHANNELS`, `PHONE PEERS`, `PHONE CALL ext [context]` and `PHONE HANGUP channel` reach the Asterisk PBX in the Debian guest. `MQTT PUB topic message` publishes a message (or, with `(FILE fn ft`, each line of a file) to the mosquitto broker; `MQTT SUB topic [n [secs]]` waits for n messages. The broker can be another one: `MQTT_HOST` in `/etc/vmspool.d/mqtt.conf`.');
H2('NICOF: sockets, FTP and PC files');
P('NICOF (devhawala, public domain) gives CMS a BSD-style socket API (`NICOFLIB TXTLIB`, `SOCKET H`), the FTP server `CMSFTPD` and the commands `NHFS` and `HACC`, which copy files between CMS and the PC. The sockets are the PC\u2019s own: a Java program on the PC does the network work and talks to the service machines NICOFPXY and TCPIPPXY, which AUTOLOG1 starts at IPL, over one 3270 line each (03CD, 03CE).');
SCREEN(['sh vm370plus/nicof/nicof.sh start', 'nicof: started (pid 2174, log nicof.log)'], 'Starting the PC half of NICOF (Java on the PATH)');
P('On CMS, `NCFACC` accesses NICOF\u2019s disk as N. `NHFS LIST`, `NHFS GET fn ft (REPLACE` and `NHFS PUT fn ft` work on the files in `vm370plus/nicof/nhfs/<user>` on the PC. `CMSFTPD -p 2121 -usecmscmds` serves the user\u2019s disks to any FTP client on the PC\u2019s port 2121 until the client quits; each disk is a directory (a, b, ...). `-pwd x` asks for a password.');
P('The modules on NICOF’s disk are built with CE’s GCC and run under `IPL CMS`. Under the EC-mode CMS (`IPL 290`) use the versions on the language disk, built with GCC 13 (`LANG` first): `CMSFTP31` (the FTP server, same options as CMSFTPD), `NHFS31` and `HACC31` (the same commands as NHFS and HACC), `IPLOOK31 name` (a name or address lookup through the PC), `HTTPD31` and `NCTEST`. CMS cuts every argument to 8 characters, so `IPLOOK31 localhost` asks for `localhos`; `IPLOOK31 -ME` gives the PC’s own name.');
H3('HTTPD: a web server for a CMS disk');
P('`HTTPD31 -p port [-d fm] [-n count] [-k key] [-v]` serves the files of a CMS disk (default A) on the PC’s port: `GET /fn.ft` sends `fn ft`, text files as ASCII lines and GIF, PNG, JPG, ICO, PDF, ZIP and BIN files as their bytes; `GET /` sends `INDEX HTML` if the disk has one, otherwise a list of its files. `-n count` ends the server after so many requests, `-k key` lets `GET /.stop/key` end it. `INDEX HTML` on the language disk is a small example page. (`HTTPD` for IPL CMS, built with CE’s GCC, is in `vm370plus/nicof/httpd`.)');
SCREEN(['httpd31 -p 8080 -k bye', 'HTTPD: serving the A disk on port 8080'], 'A web server on CMS; on the PC, http://localhost:8080/');
P('A program that ends abnormally (HX, an abend) leaves its sockets in NICOF’s proxy: a server port stays in use until `nicof.sh stop` and `start` on the PC. End HTTPD with `-n` or `-k` when you can.');
H2('TELL');
P('`TELL userid message` sends a message to a VM/370 user (CP MSG). `TELL user AT LINUX message` sends it to a user of the Debian guest through the spool bridge: it appears on that user\u2019s terminals, and is kept until the user reads it with `tell`. `TELL ALL AT LINUX message` reaches everybody logged on to Linux. A Linux user answers with `tell userid message`, which arrives on CMS as `MSG FROM MAINT: (user) message`.');
P('`TELL user AT NET message` reaches a user on the internet. The message is published on the MQTT broker of the Debian guest as topic `vm/tell/out/user`, which the user reads with any MQTT client. The user answers by publishing to `vm/tell/from/user/to/userid`; it arrives as `MSG FROM MAINT: (net:user) message`. Outside users need a login (`tellnet-user name password` in the guest) and may only use their own TELL topics. On the PC, `tools/netfwd.py --tls cert key 8883 10.1.1.2:1883` makes the broker reachable, encrypted; the router has to forward that port to the PC. `tools/nettell.py` is a small client for the outside user.');
SCREEN(['mqtt sub vm/test 1 20', 'vm/test hello from linux', 'Ready;'], 'A message from Linux arrives on CMS');
H2('RXPIP: REXX packages');
P('RXPIP is the package manager of rexxpository: REXX routines are published as packages, and a program names what it needs with `/* REQUIRE pkg */` comments. The repository server and the rxpip client run in the Debian guest; the bridge service `rxpip` serves CMS users.');
P('`RXPIP AVAIL`, `RXPIP SEARCH term` and `RXPIP SHOW name` ask the repository. `RXPIP BUILD fn` takes `fn REXX`, installs the packages it requires and writes `fn EXEC A`, a standalone program with the packages merged in; `RXPIP RUN fn [args]` builds and runs it. `RXPIP GET name` fetches a package as `file REXX A`.');
SCREEN(['rxpip run demo', 'rxpip: mathutils 1.0.0 already installed', 'rxpip: greetutils 1.0.0 already installed', 'RXPIP: DEMO EXEC A written, 71 lines', 'HELLO, WORLD!!!', 'square of 7 is 49', 'Ready;'], 'A REXX program with its packages');
H2('Planned service machines');
TABLE(['Machine', 'Purpose'], [
  ['ASKSERV', 'an AI gateway: a Linux guest that relays questions to a language model on the PC or in the cloud; CMS users ask it through the spool, like CHAT'],
  ['MQ', 'a queue manager on CMS'],
  ['Network pack', 'optional: a firewall guest (BORDER), an HTTP load balancer, and a machine that starts more web guests under load'],
  ['SMAPI', 'a subset of z/VM\u2019s systems management API'],
], [2200, 7438], 'Planned service machines');

// ---------------------------------------------------------------- chapter 7
H1('cREXX on VM/370plus');
P('cREXX is a REXX implementation built as a compiler (RXC), an assembler (RXAS), a disassembler (RXDAS) and a virtual machine (RXBVM, RXVM). It needs more than 16 MB to compile larger programs, which is where VM/370plus came from.');
H2('The 2022 release, built on VM/370plus');
P('CMSUSER’s disks hold the 2022 cREXX tree (F0041). In the EC-mode CMS it is compiled by GCC380 with GCCLIB31 and runs in 31-bit mode with its heap above 16 MB:');
SCREEN(['logon cmsuser', 'cp define storage 256m', 'cp link maint 290 290 rr', 'ipl 290',
        'exec crxmk31', '...', 'rxcn basic', 'rxasn basic', 'rxbvmn basic', '0.1', 'Ready;'], 'Building and running cREXX on VM/370plus');
P('`CRXMK31 EXEC` compiles the tree and links `RXCN`, `RXASN`, `RXBVMN` and `RXDASN` (about 90 seconds of CPU time). `GCLB31 EXEC` builds the GCCLIB31 library itself. The output equals that of the cross-compiled build line for line.');
H2('The 24-bit build');
P('The same tree also builds with GCC370 as a 24-bit program (`rxc`, `rxas`, `rxdas`, `rxbvm` on CMSUSER’s E disk); in the EC-mode CMS with `global txtlib gcclib`, `rxbvm -l d ascommon` and `rxbvm -l d asebcdic` run their test sets.');
H2('Cross-compiled programs');
P('`rxc31`, `rxas31` and `rxbvm31` on CMSUSER’s A disk were compiled on the PC with GCC 13 (`-m31`), newlib and a small CMS runtime (`vm370plus/m5f/`). They need Hercules 4.x.');
SCREEN(['ipl 290', 'rxc31 basic', 'rxas31 basic', 'rxbvm31 basic', 'libctest a b'], 'Cross-compiled cREXX');
H2('The current cREXX release');
P('The current release (1.0.0-beta.3) needs a C99 compiler; GCC380 is GCC 3.2.3. It runs on VM/370plus cross-compiled with GCC 13 (`RXC8`, `RXAS8`, `RXBVM8`, milestone M8.1). `RXC8` needs `LIBRARY RXBIN` and `RXCEXITS RXBIN` on the disk named by `-l`; source files are `fn CREXX`:');
SCREEN(['rxc8 -l a -i a hello', 'rxas8 -l a hello', 'rxbvm8 -l a hello', 'Hello from current cREXX on VM/370plus', 'sum 1..10 = 55', 'Ready;'], 'The current cREXX on CMS');
P('Milestone M8.2 builds the same release natively with GCC380 and GCCLIB31 (`RXC82`, `RXAS82`, `RXBVM82`). The three work for programs that call no library functions; the library itself does not build natively yet (Chapter 9). Until it does, use the M8.1 tools. Native cREXX on CMS runs in EBCDIC (IBM-1047) throughout, as IBM REXX does.');
H2('Languages written in cREXX');
P('Several languages are implemented as cREXX programs. They are compiled once on the PC and run unchanged on CMS (through `RXBVM8`) and on Linux (through `rxvm`). Floating point comes from `rxfloat`, a cREXX version of the cREXX float plugin, so no native plugin is needed.');
TABLE(['Language', 'CMS', 'Linux', 'Notes'], [
  ['BASIC', '`BASIC fn.bas`', '`basic fn.bas`', 'classic, MS and ANSI dialects (`-MS`, `-FB`, `-ANSI`)'],
  ['SNOBOL4', '`SNOBOL fn.sno`', '`snobol fn.sno`', 'patterns, tables, arrays'],
  ['Pascal', '`PASCAL fn`', '`pasc fn.pas`', 'compiles Pascal to cREXX, then runs it'],
  ['Lisp', '`LISP fn.lisp`', '`lisp fn.lisp`', 'a Common Lisp subset; without a file, a read-eval-print loop'],
  ['Smalltalk', '`SMALLTLK fn.st`', '`smalltalk fn.st`', 'Smalltalk-80 with GNU Smalltalk bracket syntax; class library in `*.ST`'],
  ['PL/M', '`PLM fn`', '`plmc fn.plm`', 'PL/M-80 compiled to cREXX; `( COMPILE` or `-c` compiles only'],
  ['Prolog', '`PROLOG`', '`prolog`', 'consult files with `consult(\'family.pl\')?`'],
  ['LOGO', '`LOGO fn.logo out`', '`logo fn.logo out.svg`', 'turtle graphics written as SVG'],
], [1400, 2100, 2100, 4038], 'Languages in cREXX');
P('On CMS the languages arrive as one reader deck (`LANGS`); `READCARD *` puts the files on your A disk and `LANGSUP EXEC` rebuilds the modules from their hex form. Use one of CMSUSER’s large disks as A for it (for example `ACCESS 196 A`). CMS cuts a command argument to eight characters, so the EXECs pass file names through the program stack: type the names as usual.');
P('The languages need an EC-mode CMS with a large virtual machine: `CP DEFINE STORAGE 256M` and `IPL 290`. Their storage comes from above 16 MB through HIGHSTOR; Smalltalk alone uses about 100 MB, so it is practical only when Hercules gives VM/370plus real storage to match (the kit’s `MAINSIZE 256`); with 16 MB of real storage CP pages it to a crawl. Programs cross-built for CMS run on an 8 MB stack.');
P('On Debian the languages are part of the cREXX bundle (`lxcrexx.tgz`), unpacked under `/usr/local`; the examples are in `/usr/local/share/`*language*.');
H2('Turbo CREXX');
P('Turbo CREXX is a small Turbo-Pascal-style front end: a menu that creates, edits, compiles and runs one cREXX program at a time. It does not implement REXX; it calls the real cREXX tools.');
TABLE(['Key', 'Action'], [
  ['N', 'new program (from a template), then edit it'],
  ['O / E', 'open an existing program; edit it'],
  ['C', 'compile: compiler and assembler, no run'],
  ['R', 'compile and run'],
  ['L', 'copy an example (hello, fibfact) to your disk or directory'],
  ['D', 'show the versions of the tools'],
  ['Q', 'quit'],
], [1400, 8238], 'Turbo CREXX menu');
P('**On Debian**, type `turbocrexx` (or `turbocrexx` *file*`.crexx`). The editor is `$EDITOR`, else nano or vi. Underneath, `crexx` *file*`.crexx` compiles and runs a program (`crexx -noexec` only compiles): a small driver of VM/370plus for `rxc`, `rxas` and `rxvm`, since the upstream `crexx` driver is not yet built for s390.');
P('**On CMS**, type `TURBO` (or `TURBO fn`). Programs are `fn CREXX A`; the editor is EDIT; compile and run use `RXC8`, `RXAS8` and `RXBVM8` with `LIBRARY RXBIN` on A. Without the menu: `TURBO C fn`, `TURBO R fn`, `TURBO E fn`. The examples are `HELLO TCEXAMPL` and `FIBFACT TCEXAMPL`.');
SCREEN(['turbo', '  T U R B O   C R E X X   ---   VM/370plus CMS', '  Current file: (none)',
        '  N New    O Open    E Edit    C Compile    R Run', '  L Examples         D Versions             Q Quit', 'Command:'], 'Turbo CREXX on CMS');

// ---------------------------------------------------------------- compilers
H1('Compilers on CMS');
P('VM/370plus runs current compilers natively on CMS: GCC 13 for C and C++, GnuCOBOL 3.1.2 for COBOL, and a PL/I translator. They need the EC-mode CMS (`IPL 290`), a large virtual machine (`CP DEFINE STORAGE 256M`) and Hercules 4. Their headers and libraries come from two tar files on an accessed disk, `SYSROOT TAR` (C) and `CXXROOT TAR` (C++); programs see them as `/usr/include` and `/usr/lib`, read only.');
H2('The language disk: LANG');
P('All the compilers and interpreters of this chapter are on one disk, MAINT 19F (volume VMLANG, a 3380 at real address 182, attached to the system by AUTOLOG1 at every IPL). `LANG` (on the Y disk) links it read-only and accesses it as L; `LANG fm` uses another letter. A file of the same name on one of your own disks comes first in the search order, so erase old copies of these EXECs and MODULEs from your A disk. `TYPE LANG README L` lists what is there.');
SCREEN(['cp def stor 256m', 'ipl 290', 'lang', 'L (19F) R/O', 'LANG: language disk 19F is L -- HELP: LANG README', 'Ready;'], 'Accessing the language disk');
H2('C and C++: the CC EXEC');
P('`CC fn` compiles `fn C` with GCC 13 and links `fn MODULE`; a file type of `CPP`, `CC` or `CXX` (or the option `CPP`) selects C++. The steps are those of GCC on Linux: the compiler proper (`CC1 XIMAGE` or `CC1PLUS XIMAGE`, run from above 16 MB by the `XL` loader), the GNU assembler (`AS`) and linker (`LD`, binutils 2.42), then `ELF2CMS`, which turns the linked program into a TEXT deck for `LOAD` and `GENMOD`.');
TABLE(['Option', 'Effect'], [
  ['`OPT -O0` … `-O2`', 'optimization (default `-O2`)'],
  ['`CPP`', 'C++ for any file type'],
  ['`NOLINK`', 'stop after `fn O`'],
  ['`LIB name`', 'also link `/usr/lib/libname.a` from the sysroot'],
], [2400, 7238], 'CC EXEC options');
SCREEN(['cp define storage 256m', 'ipl 290', 'access 1b0 a', 'cc primes', 'primes 2000',
        'PRIMES: 303 primes up to 2000, sum 277050, avg 914.356', 'Ready;'], 'Compiling a C program on CMS');
P('Programs are compiled for 31-bit addressing, z900 instructions and software floating point (IEEE, not hexadecimal), with newlib as the C library: the same as the programs cross-compiled on the PC (Chapter 7). Inside a program, characters are ASCII; the run-time converts to EBCDIC at the CMS file and terminal boundary. File names are CMS file ids: `fopen("data.txt", "r")` opens `DATA TXT` on any accessed disk.');
P('C++ has the standard library of GCC 13 (strings, containers, algorithms, iostreams) in the C locale. There are no exceptions: a `throw` ends the program with a message. Static destructors do not run at exit.');
P('A compile of a small program takes about half a minute of CPU time. Keep the work files on a large disk: the compiler writes `fn S`, `fn O` and `fn ELF` beside the source and erases them when it is done.');
H2('COBOL: GnuCOBOL');
P('`COBOL fn` compiles `fn COB` to `fn MODULE`: `COBC MODULE` (GnuCOBOL 3.1.2) translates the program to C, and the CC EXEC compiles and links it with libcob and GMP from `SYSROOT TAR`. `COBOL fn ( C` stops after `fn C`. Subprograms are linked statically (`CALL` of a literal name).');
SCREEN(['cobol hello', 'hello', '      HELLO FROM GNUCOBOL ON VM/370PLUS', 'SUM 1..1000 = 000000000000500500',
        'INTEREST =         89.51', 'Ready;'], 'COBOL on CMS');
P('Files: SEQUENTIAL, RELATIVE and SORT work files are binary CMS files; LINE SEQUENTIAL files are text, converted between ASCII and EBCDIC. `SELECT f ASSIGN TO "rel.dat"` names `REL DAT A`. INDEXED files and the SCREEN SECTION are not available. On Debian, GnuCOBOL is a package of its own (`gnucobol-3.1.2-s390.tgz`): `cobc -x prog.cob` compiles with Debian’s gcc.');
H2('COBOL with OO: COBX');
P('`COBX fn` translates `fn COB` with `COB2C MODULE` (mikebeer/cobol: COBOL-85 with classes, methods and `INVOKE`, indexed files, SORT, free format and user functions) into C89 and compiles it with the CC EXEC to `fn MODULE`; `COBX fn ( C` stops after `fn C`. The run time is one header, `COBRT H`. COPY books are `name CPY` files. On Debian the same translator is `cobol`: `cobol prog.cob -o prog.c`, then gcc.');
H2('PL/I');
P('The Community Edition’s IBM PL/I (F) compiler is on the Y disk (`PLI fn`, then `GLOBAL TXTLIB PLILIB` and `LOAD fn (START`). VM/370plus adds a PL/I-to-C translator (mikebeer/pli): `PLIC fn` translates `fn PLI` to C with `PLI2C MODULE` and compiles it with the CC EXEC to `fn MODULE` (`PLIC fn ( GCC380` uses GCC380 instead). SYSIN is the terminal and the console stack; a line of just `/*` is end of file. PL/I file names become CMS file ids: `data/plitest_out.txt` is `PLITTOUT TXT` (a part longer than eight characters keeps its first and last four). On Debian, `plic fn.pli` translates and compiles with gcc.');
H2('Python');
P('`PYTHON fn [args]` runs `fn PY` with the C89 Python 3 interpreter of mikebeer/python; `PYTHON` alone reads statements from the terminal. NumPy, pandas and SciPy (the interpreter’s own versions), re, json, struct, cmath, complex numbers, classes, generators and exceptions are built in. The interpreter (`PY XIMAGE`, 2.3 MB) runs above 16 MB through the XL loader. Integers beyond ±999,999,999 are big integers. There are no sockets on CMS yet, and modules of your own are `name PY` files on an accessed disk.');
H2('Pascal');
P('`PCPAS fn` compiles `fn PASCAL` with the New Stanford Pascal compiler (2026.10, Bernd Oppolzer) into P-code (`fn PCODE`), and `PCRUN fn` runs it with the P-code interpreter PCINT. Files are passed as `DD_x=val` arguments (PCRUN), as the compiler documents. CE\u2019s own Pascal (`PASCAL` on the Y disk) is unchanged.');
H2('APL');
P('`APL fn.apl` runs an APL script, `APL` alone is the interactive session (type `)OFF` to leave). It is mikebeer/apl, a C89 APL interpreter that writes primitives as words (`rho`, `iota`, ...), so it needs no APL keyboard; the module is `APLM`.');
H2('xBase and Smalltalk');
P('`XBASE fn.prg` runs a Clipper/dBASE-style program (mikebeer/xbase) with the C89 interpreter `XBASEC XIMAGE` (through the XL loader), or with the cREXX one when only `XBASE RXBIN` is there. Tables are `name DBF` files with `DBT` memos and `XDX` indexes. `SMALLTLK fn.st` runs a Smalltalk-80 program (mikebeer/smalltalk, GNU Smalltalk bracket syntax) with the C89 interpreter `SMALLTC MODULE`, or the cREXX `SMALLTLK RXBIN`; `SMALLTLK -i` is the interactive prompt. Its class library is the `ST` files `KERNEL`, `NUMBERS`, `COLLECTI` and so on. SmallIntegers are ±2^30 on CMS; larger numbers are big integers. Both are on Debian as `xbase-c` and `stc`.');
H2('ZIP, UNZIP and GZIP');
P('`ZIP zipfile files` and `UNZIP zipfile` are Info-ZIP zip 3.0 and unzip 6.0, so archives travel between CMS and the PC: text files are stored in ASCII, and CMS dates become the archive dates. `UNZIP -l` lists, `-t` tests and `-o` overwrites. A file extracts to its `fn.ft` name, as text unless its file type is a binary one (ZIP, GZ, TAR, DB, O, ...). `GZIP fn.ft` writes `fn ft-GZ` in gzip format, and `GZIP -d fn.ft-gz` restores the file. Encryption is not included. CE\u2019s MINIZIP and MINIUNZ remain on the Y disk.');
SCREEN(['zip notes.zip report.txt data.csv', '  adding: report.txt (deflated 61%)', '  adding: data.csv (deflated 48%)', 'Ready;', 'unzip -l notes.zip', 'Archive:  notes.zip', '...'], 'ZIP on CMS');
P('`UNTERSE fn.trs out.ft` unpacks an IBM TERSE file (PACK or SPACK, as AMATERSE or TRSMAIN write it on z/OS) into a CMS text file, one record per line; `UNTERSE -b` keeps the bytes (variable records get their 4-byte RDW), for binary data such as load modules or dumps; `UNTERSE -i` shows the header (PACK/SPACK, RECFM, LRECL). It is the Open Mainframe Project\u2019s tersedecompress, in C. `TERSE fn.ft out.trs` goes the other way: SPACK format, RECFM F with the longest line as LRECL (`-l n` sets it, `-v` makes V records); `TERSE -b -l n` packs binary data. The encoder runs the decoder\u2019s own model and compresses as well as IBM\u2019s SPACK.');
H2('File names in C programs');
P('Programs built with the CMS runtime (all those of this chapter) use Unix file names and map them to CMS files: `fn.ft` is `fn ft *` for reading (any accessed disk) and `fn ft A` for writing; `fn.ft.fm` and `x/fn.ft` give the mode; `/tmp/fn.ft` is a scratch file on A; a name without a dot is `FILE name`, as FILEDEF defaults it; parts longer than eight characters are cut to eight. Text files are converted between ASCII and EBCDIC (IBM-1047); file types such as `O`, `TEXT`, `TAR`, `ZIP`, `GZ`, `DB`, `DBF` and `BIN` are binary and passed through unchanged. Absolute paths under `/usr` are read from SYSROOT TAR. File dates come from the CMS file status table.');
H2('SQLite');
P('`SQLITE fn.db` starts the sqlite3 shell (SQLite 3.46) on the database `fn DB`; `SQLITE fn.db` *statement* runs one statement or dot command, for example `SQLITE emp.db .read load.sql` for a script `LOAD SQL`. A database is one binary CMS file; its rollback journal is `fn DB-JOURN` while a transaction is open. One user at a time: there is no locking between virtual machines, so give each database to one user, or to the SQLSERV service machine.');
P('**SQLSERV.** A service machine that owns the databases and runs requests for other users one at a time. `SQLSERV` waits for reader files (`WAKEUP`); each request is punched to it, run by `SQLRUN` against `name DB`, and the answer is punched back to the sender (its ORIGINID). From any user: `SQL name` *statement* runs one statement, `SQL name (FILE fn ft fm` a script, and `SQL STOP` ends the server; `(SERVER userid` names another server than SQLSERV. Results are rows with columns joined by `|`, then `SQLRUN: n statements, n errors`. At the server console, `STOP` ends it; `SQLSERV (LOUD` logs each request.');
SCREEN(['sql emp select count(*) n, sum(salary) total from emp', 'n|total', '3|25100.0', 'SQLRUN: 1 statements, 0 errors', 'Ready;'], 'A request to SQLSERV');
SCREEN(['sqlite emp.db .read sqltest.sql', 'dept   n  avg_sal  top', '-----  -  -------  ------', 'Ops    2  7000.13  7100.0', 'R&D    2  9000.25  9200.5', 'Ready;'], 'SQLite on CMS');
H2('CP/M 2.2');
P('`CPM` starts CP/M 2.2 in your CMS machine (RunCPM, a Z80 emulator; after `LANG`, under `IPL 290` in more than 16 MB). CP/M drive X: is the CMS disk X: A: is your A disk, and L: the language disk with the CP/M programs PIP, STAT, ASM, LOAD, DDT, ED, DUMP, SUBMIT and others. `EXIT` leaves CP/M. `CPM command; command` runs CP/M commands and returns. A CP/M file is the plain bytes of a CMS file of the same name, so a CMS text file must be converted first: `CPM IMPORT fn ft` makes it a CP/M text file (ASCII, CR LF), `CPM EXPORT fn ft` makes it CMS text again.');
SCREEN(['copyfile hello asm l = = a', 'cpm import hello asm', 'CPM: HELLO ASM A is now a CP/M file', 'cpm l:asm hello; l:load hello; hello', 'CP/M ASSEMBLER - VER 2.0', '012E', 'END OF ASSEMBLY', 'Hello from CP/M 2.2 on VM/370plus!', 'Ready;'], 'Assembling and running an 8080 program under CP/M on CMS');
P('The CMS terminal is line-oriented: CP/M sees each typed line, and ^C does not stop a running program. Full-screen CP/M programs work in the Debian guest instead: `runcpm` in an ssh session, or from CMS `CPM command (LINUX` through the spool bridge, with `CPM PUT fn ft (LINUX` and `CPM GET fn ft (LINUX` to move files.');

// ---------------------------------------------------------------- chapter 9
H1('Documents and Printing');
P('VM/370plus formats documents in the Debian guest through the spool bridge and gives the result back to CMS: plain text as a CMS file, PostScript as a CMS file that can be printed. Three formatters are there: troff, BookMaster markup and LaTeX.');
H2('TROFF');
P('`TROFF fn [ft [fm]] (MACROS m` formats a troff document (file type TROFF by default) with groff and the ms, me, mm, man or mom macros; the text comes back as `fn DOCUMENT A`. With `(PS` the result is PostScript, `fn PS A`.');
H2('BOOKIE: BookMaster markup');
P('BookMaster is IBM\u2019s licensed program for GML markup. BOOKIE is our own formatter for that markup: `BOOKIE fn [ft [fm]]` (file type SCRIPT by default) gives `fn DOCUMENT A`, `BOOKIE fn (PS` gives `fn PS A`. Its style is the style of this Guide: a title page and an edition notice, a contents page with page numbers, chapters on new pages, running heads and feet, screens in framed boxes, tables and figures. This Guide is itself available as `GUIDE SCRIPT` and is formatted on VM/370plus with `BOOKIE GUIDE (PS`.');
P('The tags are those of BookMaster: `:userdoc`, `:prolog` with `:title`, `:docnum` and the title-page tags `:prodname :subtitle :version :based :filenum`, `:frontm :tipage :edition :toc :body :appendix`, headings `:h1`\u2013`:h6`, `:p :note :attention`, lists `:ol :ul :sl :li :dl :dt :dd`, `:xmp` for screens, `:fig :figcap`, `:table :row :c`, highlighting `:hp1`\u2013`:hp3`, `:q`, `:xph`, symbols such as `&colon.` and `&amp.`, and the control words `.*`, `.sp`, `.br`, `.pa`, `.im`.');
SCREEN([':h1.Introduction', ':p.This book is written in :hp2.BookMaster:ehp2. markup.', ':ul.', ':li.headings, paragraphs and highlighting;', ':li.lists, screens, figures and tables.', ':eul.'], 'BookMaster markup');
H2('LaTeX');
P('`LATEX fn [ft [fm]]` (file type TEX by default) runs LaTeX (pdfTeX, TeX Live 2023) twice, for the contents and references, and dvips; the PostScript comes back as `fn PS A`. `LATEX fn (PDF` runs pdfLaTeX and leaves the PDF in the Debian guest. The packages are those of LaTeX itself, amsmath, graphics, tools, geometry and url; a run takes about a minute.');
H2('Printing PostScript');
P('`PSPRINT fn [ft [fm]]` (file type PS by default) prints a PostScript file. By default it goes to the virtual printer without page headers; CP\u2019s printer 00E writes it on the PC, where `tools/psprint.py` takes each PostScript job out unchanged and hands it to the printer (lp, or a printer\u2019s port 9100) or makes a PDF. With `(BRIDGE [format]` it goes through the spool bridge: Ghostscript in the Debian guest converts it to `ps`, `pdf`, `ljet4`, `pxlmono` or `pxlcolor` and sends it to the printer named in `/etc/vmspool.d/print.conf`.');
P('On a newly installed system the real printer 00E is drained, so spooled print files wait. An operator starts it once with `START 00E` (from MAINT, or `vmcp start 00e` in the Debian guest). The EXECs of this chapter are on MAINT\u2019s 19D disk, so they work under the usual `IPL CMS`; the samples (`SAMPLE SCRIPT`, `GUIDE SCRIPT`, `HELLO TEX`, `HELLO TROFF`) are on the language disk, which uses EDF and needs the EC-mode CMS: `IPL 290`, then `LANG`.');
SCREEN(['bookie sample (ps', 'bookie: /var/spool/vmspool/out/operator/sample.ps, 12369 bytes, 5 page(s)', 'BOOKIE: SAMPLE PS A, 354 lines', 'Ready;', 'psprint sample', 'PRT  00E OUTPUT OF OPERATOR FILE  = 0041 RECDS= 000355 COPY= 01  A PRT', 'Ready;'], 'A document formatted and printed');
P('Ghostscript itself (`gs`, `ps2pdf`, `pdf2ps`) is in the Debian guest for Linux users.');

// ---------------------------------------------------------------- chapter 10
H1('Restrictions and Known Problems');
TABLE(['Area', 'Restriction'], [
  ['Real storage', 'CP’s frames are in the low 16 MB; storage above is a paging store. Large guests page heavily.'],
  ['Linux disk I/O', 'The initramfs still limits disk requests to 128 KB (from the time of the 64-CCW limit, now gone); `echo 760 > /sys/block/dasda/queue/max_sectors_kb` allows larger ones.'],
  ['Shared systems', 'The models of a shared named system are not freed when the last user leaves; they stay until CP is IPLed.'],
  ['CP dump', 'DMKDMP cannot write its dump under ESA/390 (I-201); after an abend the registers must be taken from Hercules.'],
  ['CMS', 'The CMS nucleus is 24-bit; programs above 16 MB use HIGHSTOR and SVC 120.'],
  ['EDF disks', 'Only under `IPL 290` (the 190 CMS sees an EDF disk as unknown). A V-format file can only be extended at its end. Commands that take record numbers (TYPE, COPYFILE FROM/FOR) reach records 1 to 65,535 directly; beyond that only in sequence. LISTFILE shows counts above 99,999 in thousands (`1400K`). The A-disk accessed at IPL must be in 800-byte format. DISK DUMP/LOAD, TAPE, MOVEFILE and FULIST are not yet checked on EDF disks.'],
  ['CPWATCH', 'Loops under this CP; not autologged.'],
  ['QUERY VIRTUAL STORAGE', 'Shows five digits.'],
  ['Multiprocessing', 'One CPU only.'],
  ['cREXX, native', 'The natively built current cREXX (M8.2) cannot build its library yet; use RXC8, RXAS8 and RXBVM8 (M8.1).'],
  ['CMS file names', 'Programs cross-compiled for CMS cut a file name longer than eight characters to eight (Smalltalk’s `collections.st` is `COLLECTI ST`).'],
  ['C and C++ on CMS', 'No C++ exceptions (a `throw` ends the program), no threads, only the "C" locale. A compile needs `CP DEFINE STORAGE 256M`; cc1plus takes one to two minutes of CPU.'],
  ['Language disk', 'An old copy of an EXEC or MODULE of the same name on one of your own disks comes first in the search order (PLIC, COBOL, APL ...); erase it.'],
  ['ZIP, GZIP', 'No encryption. A file extracted from a .zip is text unless its file type is binary. GZIP writes `fn ft-GZ` (CMS has no second dot).'],
  ['TERSE', 'Files packed by TERSE read back with UNTERSE; that z/OS AMATERSE reads them has not been tested yet.'],
  ['Smalltalk, xBase', 'With a 32-bit `long`, Smalltalk SmallIntegers are \u00b12^30 (larger ones are big integers); CMS has no directories.'],
  ['Spool bridge', 'Requests and answers are 80-column cards: a longer line of a TeX or BookMaster source is cut.'],
  ['Linux', 'Linux 4.0 31-bit is the last kernel with 31-bit support; Debian 7 is the last Debian for s390 (31-bit).'],
], [2400, 7238], 'Restrictions');

// ---------------------------------------------------------------- appendices
setAppendix();
H1('Command Summary');
H2('SET ESA');
SCREEN(['SET ESA ON', 'SET ESA OFF'], 'SET ESA');
P('Class G. **ON**: the next IPL of a device (not an IPL by name) makes the virtual machine an ESA/390 machine; it stays pending across `IPL CMS`. **OFF**: the machine is a System/370 machine at once and nothing is pending. Any operand other than OFF means ON.');
H2('SET MACHINE, QUERY MACHINE');
SCREEN(['SET MACHINE 370|XA|ESA', 'QUERY MACHINE'], 'SET MACHINE');
P('Class G; the names z/VM uses. `SET MACHINE ESA` and `SET MACHINE XA` are `SET ESA ON`, `SET MACHINE 370` is `SET ESA OFF`. `QUERY MACHINE` answers `MACHINE 370`, `MACHINE ESA`, or `MACHINE 370, ESA PENDING` while an ESA/390 IPL is pending. `SET MACHINE Z` comes with milestone M9.');
SCREEN(['cp query machine', 'MACHINE 370', 'cp set machine esa', 'cp q mach', 'MACHINE 370, ESA PENDING'], 'QUERY MACHINE');
H2('IPL');
P('As in VM/370, with three changes: `IPL LINUX` IPLs the disk at 250 as an ESA/390 machine; any other IPL by name gives a System/370 machine; an IPL of a device by a user who runs a shared named system is done with CLEAR.');
H2('QUERY SET');
P('As in VM/370, with a fifth line: `ESA ON` (an ESA/390 machine), `ESA PENDING` (`SET ESA ON` given, takes effect at the next IPL of a device) or `ESA OFF`.');
SCREEN(['cp query set', 'MSG ON , WNG ON , EMSG TEXT, ACNT ON , RUN OFF', 'LINEDIT ON , TIMER ON  , ISAM OFF, ECMODE ON', 'ASSIST OFF           , PAGEX OFF, AUTOPOLL OFF', 'IMSG ON , SMSG OFF, AFFINITY NONE   , NOTRANS OFF', 'STBYPASS OFF , ESA PENDING'], 'QUERY SET after SET ESA ON');
H2('CHAT EXEC');
SCREEN(['CHAT [question] [(WAIT n BOT name LOUD]'], 'CHAT EXEC');
P('On MAINT 19D. Sends the question to the CHATBOT service machine and types the answer; without a question it asks repeatedly until an empty line. **WAIT** is the time to wait for the answer (default 60 seconds), **BOT** another service machine, **LOUD** shows the steps. Keep your reader empty of other files while you use it. Chapter 6.');
H2('WAKEUP');
SCREEN(['WAKEUP [+hhmmss|+mmss|+ss] [(RDR QUIET]'], 'WAKEUP');
P('On MAINT 19D (new; VM/370 had none). Waits until the interval has passed (return code 2) or, with **RDR**, until a file arrives in the virtual reader (return code 3); 24 means an invalid operand. A file already in the reader does not end the wait, so look at the reader first (`QUERY READER`). WAKEUP sets `CP SET TIMER REAL` so that the interval timer runs while the machine waits. Service machines use it instead of polling.');
SCREEN(['wakeup +000005', 'Ready(00002);', 'wakeup +000500 (rdr', 'PUN FILE 0002 FROM CMSUSER  COPY 01 NOHOLD', 'Ready(00003);'], 'WAKEUP');
H2('FULIST');
SCREEN(['FULIST [fn [ft [fm]]]', 'FL [fn [ft [fm]]]'], 'FULIST');
P('On MAINT 19D; a full-screen file list in the manner of IBM’s FILELIST, written in REXX with the REXX full-screen package; it runs under `IPL CMS` and under the EC-mode CMS on 290. The default is `* * A`. Each line is an input field: type a command over the start of a line and press Enter. `/` in the command stands for the file (`fn ft fm`), `/n`, `/t`, `/m` for its parts; without a `/` the file is appended. Abbreviations: **E** or **X** edit (EE), **B** browse (FSVIEW), **D** discard (ERASE), **=** repeats the last command. The status column shows `*` or `*rc` for each command run.');
TABLE(['Key', 'Action'], [
  ['PF1', 'HELP FULIST'], ['PF2', 'refresh the list'], ['PF3, PF15', 'quit'],
  ['PF4, PF5, PF6', 'sort by type, date (newest first), size'], ['PF7, PF8', 'page back, forward'],
  ['PF9', 'FULIST of all files with the name at the cursor'], ['PF10', 'sort by name'],
  ['PF11', 'edit the file at the cursor'], ['PF12', 'cursor to the command line and back'],
], [2400, 7238], 'FULIST keys');
P('On the command line (`====>`): QUIT, TOP, BOTTOM, REFRESH, SORT NAME|TYPE|DATE|SIZE, FULIST pattern, or any CMS or CP command.');
H2('TDISK');
SCREEN(['TDISK [cyls] [fm]'], 'TDISK');
P('On MAINT 19D (new). A temporary disk in one step: CP defines it (`DEFINE T3350`) at the first free address from D00 to D1F, CMS formats it with the label TMPvaddr and, if a file mode is given, accesses it. **cyls** is 1 to 100 cylinders, default 10 (about 5 MB); without **fm** the disk is formatted but not accessed. The operands may come in either order. Return codes: 24 invalid operands, 28 the file mode is in use, 32 no free address or no TEMP space; otherwise FORMAT’s. The contents are lost at LOGOFF or DETACH. `HELP TDISK` describes it.');
SCREEN(['tdisk 5 t', 'DASD D00 DEFINED 0005 CYL', 'TDISK: disk D00 is ready as T, 5 cylinders, label TMPD00', 'Ready;'], 'TDISK');
H2('FORMAT (BLKSIZE 4096)');
SCREEN(['FORMAT vaddr fm [cyls] (BLKSIZE 4096|4K|800'], 'FORMAT');
P('Under `IPL 290` (changed). **BLKSIZE 4096** formats the disk in IBM’s Enhanced Disk Format: 4096-byte blocks, the file directory and the allocation map kept as files, records counted in fullwords. Without **cyls** the whole minidisk is used, whatever its size (FORMAT’s usual 800-byte limit, 121 cylinders on a 3380, does not apply). 3350 and 3380 disks. FORMAT asks for confirmation and a label as usual and then accesses the disk. **BLKSIZE 800** is the usual format. A disk written by `tools/edf.py` on the PC can be accessed too, and the reverse.');
SCREEN(['format 1b0 y (blksize 4096', 'DMSFOR603R FORMAT WILL ERASE ALL FILES ON DISK \'Y(1B0)\'. DO YOU WISH TO CONTINUE? (YES|NO):', 'yes', 'DMSFOR605R ENTER DISK LABEL:', 'edf002', 'FORMATTING DISK \'Y\'.', 'Ready;', 'q disk y', 'Label  CUU M  Stat  Cyl Type Blksize   Files  Blks Used-(%) Blks Left  Blk Total', 'EDF002 1B0 Y   R/W  885 3380 4096          0         10-00     132740     132750', 'Ready;'], 'FORMAT (BLKSIZE 4096)');
H2('GLOBALV');
SCREEN(['GLOBALV [SELECT group] SET|SETS|SETL|SETP name value', 'GLOBALV [SELECT group] PUT|PUTS|PUTL|PUTP name ...', 'GLOBALV [SELECT group] GET|STACK name ...', 'GLOBALV [SELECT group] LIST [name ...]|PURGE', 'GLOBALV GRPLIST'], 'GLOBALV');
P('New; a subset of z/VM’s GLOBALV. Variables live in groups (UNNAMED unless SELECT names one). **SET** keeps a value until the next IPL of CMS, **SETS** also in `SESSION GLOBALV A` (erased at the next IPL), **SETL** in `LASTING GLOBALV A` (read again at every IPL), **SETP** both. **PUT** takes the values of the calling EXEC’s REXX variables, **GET** sets them, **STACK** puts values on the console stack. A null value drops a variable. Names up to 32 characters, values up to 255, 128 variables. GLOBALV is a nucleus extension, loaded at IPL by the system profile.');
SCREEN(['globalv select fulist setl editor EE', 'globalv select fulist list', 'FULIST   EDITOR = EE'], 'GLOBALV');
P('FULIST takes its editor and browser from the group FULIST (`EDITOR`, `BROWSER`) when they are set.');
H2('LANG');
P('`LANG [fm]` links MAINT 19F read-only (if it is not linked) and accesses it as `fm` (default L).');
H2('ZIP, UNZIP, GZIP');
P('`ZIP zipfile files`, `UNZIP [-l|-t|-o] zipfile [files]`, `GZIP [-d] fn.ft` (Info-ZIP zip 3.0 and unzip 6.0; gzip format from zlib).');
H2('CPM');
P('`CPM`, `CPM command [; command ...]`, `CPM IMPORT|EXPORT fn ft [fm]`; with `(LINUX`: `CPM command (LINUX`, `CPM PUT|GET fn ft (LINUX`, `CPM DIR|RESET (LINUX`.');
H2('TERSE, UNTERSE');
P('`TERSE [-v] [-l lrecl] in out` packs a file in IBM TERSE SPACK format (`-b` binary); `UNTERSE [-b] in out` unpacks PACK and SPACK files, `UNTERSE -i in` shows the header.');
H2('SQL, SQLSERV');
P('`SQL db statement`, `SQL db (FILE fn ft fm`, `SQL STOP`, `(SERVER userid`; `SQLSERV [(LOUD]` runs the service machine.');
H2('LXCALL, PHONE, MQTT');
P('`LXCALL service [args] (FILE fn ft fm | SERVER userid | WAIT n | NOTYPE`; `PHONE STATUS|CHANNELS|PEERS|CALL ext [ctx]|HANGUP channel`; `MQTT PUB topic message`, `MQTT SUB topic [n [secs]]`; `TELL userid [AT LINUX|NET] message`.');
H2('RXPIP');
P('`RXPIP AVAIL`, `SEARCH term`, `SHOW name`, `BUILD fn`, `RUN fn [args]`, `GET name`.');
H2('TROFF, BOOKIE, LATEX, PSPRINT');
P('`TROFF fn [ft [fm]] (MACROS m PS`; `BOOKIE fn [ft [fm]] (PS`; `LATEX fn [ft [fm]] (PDF`; `PSPRINT fn [ft [fm]] (BRIDGE [format]`.');
H2('LINUX EXEC');
SCREEN(['LINUX [DISK|READER]'], 'LINUX EXEC');
P('On MAINT 19D. **DISK** (the default) issues `CP IPL LINUX`. **READER** punches `LINUX48 KERNEL1`, `KERNEL2` and `userid LXPARM` (or `DEFAULT LXPARM`) to the user’s reader, holding the other reader files, and IPLs the reader as an ESA/390 machine. Needs 64 MB and, for Debian, the devices listed in Chapter 5 under “What a Linux user needs”.');

H1('Contents of the Kit');
TABLE(['Path', 'Contents'], [
  ['`vm370ce.conf`', 'ESA/390 configuration, MAINSIZE 256, the CTC and 3390 lines for Linux'],
  ['`disks/shadows/*_1.shadow`', 'the VM/370plus system: CP nucleus, CMS, directory, user disks'],
  ['`disks/vm50-4.cckd`', 'VM50-4 with the paging area record corrected'],
  ['`disks/lnx190.cckd`', 'the Debian disk, a 3390-9 (after joining parts 6–13)'],
  ['`disks/lang.cckd`', 'the language disk VMLANG, a 3380 at address 182 (after joining parts 14–15)'],
  ['`README-VM370PLUS.txt`', 'the short form of this manual'],
  ['`vm370plus/linux/`', 'linux48.rdr, lxparm.rdr, lxparmram.rdr, kernel configuration, patches, initramfs sources, the Debian post-install script'],
  ['`vm370plus/m5f/`', 'the CMS runtime for cross-compiled 31-bit C programs'],
  ['`vm370plus/*.EXEC`, `*.C`', 'GCC31 EXEC, CRXMAKE EXEC, examples'],
  ['`vm370plus/chatbot/`', 'the CHATBOT service machine: source, data deck, the VM/370 changes (already installed on the system)'],
], [3200, 6438], 'Kit contents');

H1('ESA/390 Deviations of the Virtual Machine');
P('The ESA/390 virtual machine is complete enough for Linux. Where it deviates from the architecture, the deviation is recorded in the project’s register (documents 39 and 40). The ones a user may notice:');
B(['A format-1 channel program is converted to format 0 in place while it runs (any length; at most 32 TIC targets).',
   'No SCLP and no z/VM interfaces (DIAGNOSE codes of z/VM); Linux must be told `conmode=3215 condev=0x0009`.',
   'One processor; SIGP is answered for that processor only.',
   'Selector channels of an ESA/390 guest behave as block multiplexer channels.',
   'Linux’s machine-check handling is not exercised: CP presents no machine checks to the guest.']);

H1('CP and CMS Command Tables');
P('These tables are generated from the system itself (`tools/cmdtables.py`, document 50 in the repository): the CP commands from CP’s command table in DMKCFC, in the order CP searches it; the QUERY and SET operands from its operand lists; the CMS commands from the nucleus function table and from the MODULE and EXEC files on the system disks. **Abbrev** is the shortest form CP accepts, **Class** the privilege class (any: no class needed). **+** marks a VM/370plus addition or change.');
const md = require('fs').readFileSync(__dirname + '/../../../docs/50-COMMANDS.md', 'utf8').split('\n');
let i = 0;
while (i < md.length) {
  const l = md[i];
  if (l.startsWith('## ') && !l.startsWith('## Not yet')) { H2(l.slice(3)); i++; continue; }
  if (l.startsWith('### ')) { H3(l.slice(4)); i++; continue; }
  if (l.startsWith('| ') && md[i + 1] && md[i + 1].startsWith('|---')) {
    const cells = x => x.slice(1, -1).split(' | ').map(c => c.trim());
    const head = cells(l.trim()); const rows = []; i += 2;
    while (i < md.length && md[i].startsWith('| ')) { rows.push(cells(md[i].trim())); i++; }
    const w = head.length === 5 ? [1500, 1100, 1000, 1500, 4538] : head.length === 3 ? [1700, 1700, 6238] : [1900, 7738];
    TABLE(head, rows, w);
    continue;
  }
  if (l.startsWith('Also: ') || (/^[A-Z$].*\.$/.test(l) && l.includes(', '))) P(l);
  i++;
}
};
