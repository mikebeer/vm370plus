# VM/370plus milestone plan

The plan of record, adopted 11 October 2026 from the owner's workbook
`docs/VM370PLUS-Milestones.xlsx` (that file is the source; this page is generated from it).
Renumbering against the earlier plan: the work before 64-bit is now **M9** (9.1-9.12), 64-bit is **M10**,
and the work after 64-bit is **M11**.

| M0 |   | CP conversion | The assembler: ESA/390 instructions for CE's ASSEMBLE | Done | Sep | Closed, no listing check outstanding |
|---|---|---|---|---|---|---|
| M1 |   | CP conversion | CP IPLs in ESA/390 mode (DAT off), console, channel subsystem, DASD, dump | Done | 30 Sep–Oct |  |
| M2 |   | CP conversion | DAT on: ESA/390 segment/page tables, CP in AMODE 31, 31-bit guests | Done | Oct | AMODE 31 made consistent by I-259/I-260 on 11 Oct |
| M3 |   | CP conversion | One S/370-mode guest logs on and runs CMS | Done | 3–4 Oct |  |
| M3 | 3a | CP conversion | DASD IPL of CP (DMKSAV, DMKCKP, DMKCPI) | Done | 3 Oct | R-09 retired |
| M3 | 3b | CP conversion | CMS by IPL 190 | Done | 4 Oct | R-08 retired |
| M3 | 3c | CP conversion | Named systems shared by page frame | Done | 4 Oct | Increment 2 open (33-FRAME-SHARING) |
| M4 |   | CP conversion | Two guests isolated; 4 KB storage keys (ISKE/SSKE, DISPLAY K) | Done | Oct | Largely done; R-12 measured |
| M4 | 4b.1 | Real storage | Probe in AMODE 31; CORTABLE built at IPL from the real size | Done | 7 Oct |  |
| M4 | 4b.2 | Real storage | Storage above 16 MB as CP's paging store (MVCL slots) | Done | 7 Oct | XA0053DK, docs/38 |
| M4 | 4b.4 | Real storage | Up to 2 GB; CP's machine ends at 16 MB, the rest is paging store | Done | 7 Oct | MAINSIZE 2048 tested |
| M4 | 4b.3 step 1 | Real storage | CP stays AMODE 31 after TRANS / DMKPTRAN's LRA | Done | 11 Oct | f6f85d6 |
| M4 | 4b.3 step 2 | Real storage | IDAW consumers and real-address masks 31-bit clean | Done | 11 Oct | With steps 3–4 |
| M4 | 4b.3 step 3 | Real storage | DMKPAG paging I/O through a two-IDAW list for high frames | Done | 11 Oct | ba6c595 |
| M4 | 4b.3 step 4 | Real storage | DMKCCW / DMKDGD: one-page data areas in high frames through IDALs | Done | 11 Oct | ac962df; I-262 fixed later |
| M4 | 4b.3 step 5 | Real storage | CORTABLE to frames top, high free list, switch (frames 16–160 MB) | Done | 11 Oct | 9ed9957; I-259..I-262; regression clean at 16/256 MB |
| M4 | 4b.3 step 6 | Real storage | Two Debians at once (I-254) | Done | 11 Oct | dd6: both at login, both answer SSH; I-263, I-264 |
| M4 | 4c | Real storage | CPWATCH running again (I-251) | Planned |  | After CP leftovers |
| M5 | 5a | CMS | Build CMS from source with our pipeline | Done | 5 Oct |  |
| M5 | 5b | CMS | EC-mode CMS (IPL 290) | Done | 6 Oct | XA0051DK |
| M5 | 5c | CMS | CMS storage above the line (HIGHSTOR, CMSSTOR) | Done | 6 Oct |  |
| M5 | 5d | CMS | AMODE 31 service calls from programs | Done | 6 Oct | I-248, I-249 |
| M5 | 5e | CMS | cREXX compiles on the machine | Done | 7 Oct |  |
| M5 | 5f | CMS | 31-bit C toolchain, cross (GCC 13 -m31, newlib, cmsrt) | Done | 7 Oct |  |
| M5 | 5g | CMS | Native 31-bit C (GCCLIB31), cREXX built on the machine | Done | 7 Oct |  |
| M6 |   | CMS | 31-bit CMS proper; larger minidisks (4K blocks) | Done | 11 Oct | EDF under IPL 290 (docs/63); 31-bit nucleus sweep stays background (doc 62 B) |
| M6 | 6a | CMS | FORMAT (BLKSIZE 4096 writes an EDF label, directory and allocation map | Done | 11 Oct | 25de15c; T3350 temp disk and dedicated 3380 |
| M6 | 6b | CMS | EDF disks read: ACCESS, LISTFILE, STATE, TYPE, COPYFILE from (DMSEFS, XA0056DK) | Done | 11 Oct | 86699c4; 100,000-record V and 70,000-record F files read |
| M6 | 6c | CMS | EDF write, erase, rename; files above 65,535 records | Done | 11 Oct | 14bae66; 70,000-record copy byte-identical (edf.py) |
| M6 | 6d | CMS | Large minidisks: a full-volume 3380 as one EDF disk | Done | 11 Oct | a101a8f; full 885-cyl 3380 (518 MB); one 112 MB file, 1.4M records, verified |
| M6 | 6e | CMS | Tools across both formats (DISK DUMP/LOAD, TAPE, MOVEFILE, FULIST) | Done | 11 Oct | DISK DUMP, MOVEFILE (100,000-record V file), COMPARE, COPYFILE on EDF; DISK LOAD writes to A by design; TAPE not run (same RDBUF/WRBUF path) |
| M7 | 7.0 | ESA/390 guests | Baseline test guest (STSCH/SSCH/ESA PSW/X'B8') | Done | 7 Oct |  |
| M7 | 7.1 | ESA/390 guests | SET ESA / machine mode, ESA IPL | Done | 7 Oct |  |
| M7 | 7.2 | ESA/390 guests | Virtual channel subsystem (DMKVCS) | Done | 7 Oct |  |
| M7 | 7.3 | ESA/390 guests | ESA/390 guest DAT (DMKVAX shadow tables) | Done | 7 Oct |  |
| M7 | 7.4 | ESA/390 guests | What Linux uses besides (secondary space, STAP, SIGP, timers, DIAG 10) | Done | 7–8 Oct |  |
| M7 | 7.5 | ESA/390 guests | Linux/390 31-bit from the reader, shell on the 3215 | Done | 8 Oct |  |
| M7 | 7.6 | ESA/390 guests | BusyBox userland, CTC network | Done | 8 Oct |  |
| M7 | 7.7 | ESA/390 guests | Debian 7.11 from a 3390, SSH over CTC | Done | 8–9 Oct |  |
| M7 | 7.8 | ESA/390 guests | Linux for several users (LINUX EXEC); Linux and CMS side by side | Done | 9 Oct | Two Debians at once: done 11 Oct (M4b.3 step 6) |
| M7 | 7.9 | ESA/390 guests | Channel programs of any length for ESA/390 guests | Done | 10 Oct |  |
| M7 | 7.10 | ESA/390 guests | IPL LINUX from its disk (vmzipl) | Done | 10 Oct |  |
| M7 | 7.11 | ESA/390 guests | 3390-9 Debian disk (7 GB) as standard | Done | 10 Oct |  |
| M8 | 8.1 | cREXX | Current cREXX cross-built for CMS (RXC8, RXAS8, RXBVM8) | Done | 8 Oct |  |
| M8 | 8.2 | cREXX | Current cREXX built natively on CMS | In progress | 9 Oct– | RXC/RXAS work; RXBVM82 open |
| M9 | 9.1 | Since kit b6 | SET/QUERY MACHINE, FULIST, GLOBALV, command tables, I-257, I-258 | Done | 10–11 Oct |  |
| M9 | 9.2 | CP leftovers | I-235 (models), I-201 (DMKDMP dump), warm start (I-267, I-268), LPSW fast path above 16 MB, per-user IP (9.12a) | Done | 11 Oct | I-201, I-265, I-267, I-268, I-269 fixed; LPSW/SVC fast paths; I-235 models kept resident by design (M10 revisits sharer invalidation) |
| M9 | 9.3 | C++ | C++ on CMS (subset, cross route) | Done |  | Superseded by 9.21: full GCC 13 C++ natively on CMS, not a subset |
| M9 | 9.4 | Database | SQL: SQLite on CMS + SQLSERV; MariaDB/PostgreSQL on Debian | In progress |  | SQLite 3.46.1 on CMS (SQLITE EXEC, tc31 = PC). SQLSERV service machine done (tc47, 12 Oct): SQL EXEC requests over reader/punch, SQLRUN, errors and STOP; QUERY READER ALL userid for class D clients. Next: SQLSERV directory entry in the kit; MariaDB/PostgreSQL on Debian. languages/sqlite |
| M9 | 9.5 | Network | CMS sockets + httpd | Planned |  |  |
| M9 | 9.6 | dbase | xBase / Harbour | Done |  | mikebeer/xbase on CMS and Debian: cREXX version (XBASE RXBIN, tc37 7/7) and the C89 version (XBASEC XIMAGE via XL; tc42 7/7 = PC; 32-bit long fixes pushed upstream 7d2a188); Debian xbase-c. Harbour itself not ported. languages/xbase (mkxbasec.sh) |
| M9 | 9.7 | MQ | MQ and MQTT (CMS and Debian) | Planned |  | mosquitto on Debian done |
| M9 | 9.8 | APL | APL in C on CMS | Done | 11 Oct | Debian 34/34; CMS 33/33 (primes.apl has lines over 80 columns); e551b9e; needs Hercules 4 |
| M9 | 9.9 | REXX | ooRexx 5 on Debian | Planned |  |  |
| M9 | 9.10 | AI | AI gateway service machine (ASKSERV): CMS/Linux/TELL clients; the model runs in llama.cpp on the PC (or a cloud API), reached through BORDER | Planned |  |  |
| M9 | 9.11 | Snapshot | Kit build 7 (distributable snapshot) | Done | 11 Oct | b7, 13 parts; verified on a fresh CE (kt16): EDF, TDISK, IPL LINUX |
| M9 | 9.12 | Firewall | Network pack: per-user addresses, CP guest LAN, BORDER firewall, inside DHCP/DNS (9.12a-d) | Planned |  | After M4b.3, before M9 |
| M9 | 9.12a | Addresses | Per-user IP address at disk IPL from the directory (fixes I-266) | Done | 11 Oct | Address follows the CTC pair (62x/63x or real); vm370plus-net on both Debian disks; verified 10.1.2.2 by itself |
| M9 | 9.12b | Guest LAN | CP virtual NIC per guest on one internal segment (LCS/QETH style, like z/VM Guest LAN) | Planned |  | Needed for DHCP: CTC links are point-to-point |
| M9 | 9.12c | Firewall | BORDER service machine: only outside link; NAT outbound; port forwards to service machines (httpd 9.5). One network configuration file on MAINT: OUTSIDE DHCP (bridged LCS/tap, address from the LAN) or OUTSIDE STATIC ip/mask gw; inside subnet and addressing; EXPOSE proto port TO user\|ip port (default: nothing exposed) | Planned |  | Outside link may be bridged (LCS on tap) to get its address from the LAN DHCP |
| M9 | 9.12d | DHCP/DNS | DHCP and DNS for the inside segment as a service machine (dnsmasq on Debian, or in BORDER) | Planned |  | Needs 9.12b |
| M9 | 9.13 | Tools | Public VM/370 tools for the kit (survey 11 Oct) | Planned |  | Before kit 7 (9.11) |
| M9 | 9.13a | Pascal | New Stanford Pascal (Oppolzer, github StanfordPascal/Pascal) replaces CE 2021.03 | Done |  | New Stanford Pascal 2026.10 on CMS via PCINT (cross-built, 31-bit; big-endian fix pcint-bigendian.patch): PCPAS/PCRUN EXECs, tc33 output = PC. Native 370 code needs a PASCAL2 newer than the published 2024.02. CE 2021.03 unchanged. languages/pascal |
| M9 | 9.13b | NJE | RSCS NJE updates (Coghlan) + TCPNJE: VM/370 NJE to MVS/z/OS/Linux | Planned |  | TCPNJE is in SDL Hyperion; we run Hercules 3.13 |
| M9 | 9.13c | NJE Linux | UnixNJE (FUNET-NJE) in the Debian guest: CMS <-> Linux NJE | Planned |  | Needs 9.13b |
| M9 | 9.13d | Sockets | NICOF + CMSFTPD (devhawala, public domain): sockets API and FTP for CMS | Planned |  | Test before 9.5 (CMS sockets + httpd) |
| M9 | 9.13e | Games | Game disk: VM/370 games tape (IBM VS BASIC runtime removed), WIOS SNAKE/SLASH, CALL-OS BASIC TREK2, Zork/Dungeon | Planned |  |  |
| M9 | 9.13f | Workshop | VM Workshop 1985/86 tools that run on R6: CARD, FCOPY, CLUST/TERM macros; check DISKPRY, LEDIT, IEBCOPY, JSCAN | Planned |  | CPWATCH (85 M0083) is 4c |
| M9 | 9.14 | TELL | TELL everywhere: CMS TELL EXEC (CP MSG local, RSCS/NJE remote), Linux tell (vmcp, kernel CONFIG_VMCP) with incoming MSG captured, PC client through BORDER (CHATBOT-style gateway) | Planned |  | Owner request 11 Oct |
| M9 | 9.15 | Python | Python on CMS: the C89 implementation from mikebeer/python (in creation), built with GCC for CMS; Debian uses standard wheezy python 2.7/3.2 packages | Done |  | mikebeer/python (C89, NumPy/pandas/SciPy built in) as PY XIMAGE, PYTHON EXEC; 27 of 29 tests = CPython on CMS (tc23/24; 15_import needs package files, 24_sockets: no sockets yet). 32-bit long fix pushed to mikebeer/python (6e4a7a5). languages/python |
| M9 | 9.16 | TDISK | TDISK EXEC: TDISK [cyls] [fm] defines, formats and (with fm) accesses a temporary disk; HELP TDISK | Done | 11 Oct | Owner request 11 Oct |
| M9 | 9.17 | Tiny LLM | llama2.c-style inference of a tiny model (stories15M) natively on Debian/390 and on CMS (C, high storage, HFP/BFP and byte order converted): a demonstration; real models via 9.10 | Planned |  | Owner request 11 Oct; llama.cpp itself needs 64-bit, C++17 and GBs |
| M9 | 9.18 | Mail | SENDMAIL / GETMAIL for CMS via a MAILER service machine (SMTP out through BORDER; POP/IMAP or SMTP in, delivered to the reader in class M) | Planned |  | Owner request 11 Oct; after 9.12 |
| M9 | 9.19 | COBOL | GnuCOBOL 3.1.2. Debian: cross-built cobc + static libcob + GMP 6.3 against wheezy glibc, gcc 4.6 compiles the generated C (done 11 Oct). CMS goal: compile natively -- first cobc/libcob through the cross route (GCC -m31, newlib, cmsrt), then built on CMS by GCC380/GCCLIB31 with a COBOL EXEC (cobc -C, GCC, LOAD, GENMOD) | Done | 11 Oct– | GnuCOBOL: Debian (cb1) and CMS natively (COBOL EXEC, tc17). Plus mikebeer/cobol (COBOL-85 + OO, indexed files): CMS COBX EXEC 19/19 = PC (tc41-44; record files binary via cobrt-cms.patch); Debian 20/20 (pd2/pd3, languages/common/mkdebtests.sh). languages/cobol, languages/cobol2c |
| M9 | 9.20 | PL/I | PL/I beyond CE's PL/I(F): research first -- no maintained open PL/I for s390 (Iron Spring is x86 and closed, pl1gcc unfinished); options: document PL/I(F), a PL/I subset to C translator, old subsets (PL/I-G) | Done | 11 Oct– | mikebeer/pli 79eae4d (Enterprise PL/I subset to C89): CMS PLIC EXEC through GCC 13, 34/35 = PC (tc43-45); Debian 38/39 incl. multi-file and separately compiled modules (pd2/pd3). Only difference both places: t32 AREA count depends on pointer size. CE PL/I(F) also works. docs/64-PLI.md, languages/pli |
| M9 | 9.21 | C/C++ | Modern C/C++ toolchain on CMS: GCC 13.3 cc1/cc1plus and binutils 2.42 (as, ld, ar) cross-built for the M5f runtime; XL loader for programs above 16 MB; sysroot tar as read-only VFS; CC EXEC; then COBOL (9.19) compiles natively | Done | 12 Oct– | Owner decision 12 Oct (GCC380 cannot add long longs). GCC 13.3 cc1/cc1plus (XL loader, above 16 MB), binutils 2.42 as/ld/ar, elf2cms, CC EXEC; SYSROOT TAR + CXXROOT TAR; libstdc++ 13.3 built for newlib/soft float. Native C (tc15/16), C++ incl. iostreams (tc20/21). No C++ exceptions yet. toolchain/ |
| M9 | 9.22 | UNTERSE | TERSE/UNTERSE for CMS and Debian: a C89 TERSE (PACK/SPACK) decompressor after IBM's open-source one, then compression; built with GCC 13 for CMS | Done |  | UNTERSE: C89 port of the Open Mainframe Project's tersedecompress (Apache 2), PACK/SPACK, F/V, text/binary; testdata 120/120; CMS byte-identical to z/OS originals (tc72). TERSE: new SPACK encoder running the decoder's model (longest known phrase), sizes = IBM SPACK within block padding; PC round trip 26 text + 3 binary, CMS round trip (tc74). TRS/UNTRS MODULE + TERSE/UNTERSE EXEC on the LANG disk; Debian terse/unterse. Open: confirm AMATERSE on z/OS reads our SPACK |
| M9 | 9.23 | ZIP | Info-ZIP zip/unzip and gzip built natively for CMS (GCC 13, cmsrt), beside CE's MINIZIP/MINIUNZ and VMARC | Done |  | Info-ZIP zip 3.0 / unzip 6.0 and gzip (zlib 1.3.1 minigzip) for CMS: IZZIP/IZUNZIP/MINIGZ MODULE + ZIP/UNZIP/GZIP EXEC on the LANG disk; round trip on CMS, PC-made .zip unzips (tc64-71); CMS file dates in the archive (cmsrt stat reads the FST via STATE). gzip names fn ft-GZ. No encryption. languages/zip |
| M9 | 9.24 | Snapshot | Kit b8: LANG disk (MAINT 19F, VMLANG) with all CMS languages, LANG EXEC, Guide chapter 8, Debian language bundle installed | Done | 12 Oct– | b8, 15 parts (14-15: LANG disk with JOINLANG); verified on a fresh CE (kt17-21): VMLANG attached by AUTOLOG1, LANG -> L (19F), SQLite 3.46.1, CC HI -> Hi!, Debian login. CMSUSER A cleaned of old language EXECs/MODULEs |
| M9 | 9.25 | Spool bridge | Linux services on the service-machine interface: vmspool daemon in the Debian guest (vmur reader/punch) turning requests into Asterisk AMI (PHONE EXEC), MQTT, MariaDB calls; replies punched back like SQLSERV | Done |  | vmspoold (C) in the Debian guest + LXCALL/PHONE/MQTT EXECs on CMS; services echo, sys, phone (Asterisk), mqtt; verified pd15; needed CP fix I-276 |
| M9 | 9.26 | RXPIP | mikebeer/rexxpository (RXPIP, pip-style REXX packages) on VM/370plus: 1) rxpip (Regina) in the Debian guest; 2) TAR for CMS; 3) RXPIPSRV service machine + RXPIP EXEC over reader/punch via the 9.25 bridge; 4) CMS install as EXEC files (external functions), CMSNAME map; platform tags brexx-cms-s390, crexx-cms-s390, regina-linux-s390 | Planned |  | Owner question 12 Oct |
| M9 | 9.27 | LaTeX | TeX/LaTeX in the Debian guest (texlive-base, dvips, pdflatex); LATEX EXEC on CMS over the 9.25 bridge: fn TEX in, fn PDF/PS/DVI back as files | Planned |  | Owner request 13 Oct |
| M9 | 9.28 | troff | groff/troff (man, ms, mm) in the Debian guest; TROFF EXEC on CMS over the bridge; text output to the CMS console or printer, PS/PDF as files | Planned |  | Owner request 13 Oct |
| M9 | 9.29 | Ghostscript, ps2pdf | Ghostscript in the Debian guest: PS->PDF (ps2pdf), PDF->PS (pdf2ps), PS/PDF->PCL/raster for printers; PS2PDF EXEC on CMS | Planned |  | Owner request 13 Oct |
| M9 | 9.30 | BookMaster | A BookMaster-compatible formatter (BookMaster is IBM-licensed; we build our own): :userdoc./:h1./:p./:ol./:dl. GML tags to SCRIPT-style line output, PS and PDF (via 9.29); BOOKIE EXEC on CMS | Planned |  | Owner request 13 Oct |
| M9 | 9.31 | Graphical printing | PS/PDF from CMS or Linux to a real printer: (a) bridge: PSPRINT EXEC -> Debian -> Ghostscript -> CUPS/LPD/IPP/port 9100 printer on the PC LAN (needs routing/NAT on the PC for the guest's CTC network); (b) spool: PostScript text on a virtual printer, Hercules 1403 to a file or socket on the PC, a PC script hands it to the printer (PS only, lines up to 132; PDF goes through (a) or ps first) | Planned |  | Owner request 13 Oct |
| M10 |  | 64-bit | z/Architecture (64-bit) | Planned |  |  |
| M10 | 10.1 | Documentation | Migration documentation 31 -> 64 bit |  |  |  |
| M10 | 10.2 | Documentation | Manual Update |  |  |  |
| M10 | 10.3 | Snapshot | Kit 8 |  |  |  |
| M11 | 11.1 | Multiprocessor-Support | MP (CP on several CPUs, virtual MP) |   |  |  |
| M11 | 11.2 | More Languages | GNU APL, full C++, Java on CMS, NetRexx 4 |   |  |  |
| M11 | 11.3 | VoIP | FreeSWITCH |   |  |  |
| M11 | 11.4 | IoT | Node-RED |   |  |  |
| M11 | 11.5 | Container | Docker, QEMU/ReactOS,  Bochs/DOS |   |  |  |
| M11 | 11.6 | CF | CFSERV |   |  |  |
| M11 | 11.7 | Documentation | Manual Update |  |  |  |
| M11 | 11.8 | Snapshot | Kit 9 |  |  |  |
| M12 | 12a | DOS/VSE | DOS/VS Release 34 installed in a virtual machine (system packs, directory entry, IPL) | Planned |  | Owner request 11 Oct; S/370-mode guest |
| M12 | 12b | DOS/VSE | DOS/VS under our CP as a regression of the S/370 guest paths (SIO/TIO, CCW translation, V=V) | Planned |  |  |
| M12 | 12c | DOS/VSE | POWER spooling through VM reader/punch; jobs submitted from CMS | Planned |  |  |
| M12 | 12d | DOS/VSE | CMS/DOS simulation cross-checked against real DOS/VS | Planned |  |  |
| M12 | 12e | DOS/VSE | Research: inventory DOS/360 R26 and DOS/VS R34 tapes for source; feasibility of a 31/64-bit DOS | Planned |  | srorso/dos360install |
