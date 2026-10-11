# VM/370plus milestone plan

The plan of record, adopted 11 October 2026 from the owner's workbook
`docs/VM370PLUS-Milestones.xlsx` (that file is the source; this page is generated from it).
Renumbering against the earlier plan: the work before 64-bit is now **M9** (9.1-9.12), 64-bit is **M10**,
and the work after 64-bit is **M11**.

| Milestone  | ID | Area | Content | Status | When | Notes |
|---|---|---|---|---|---|---|
| M0 |   | CP conversion | The assembler: ESA/390 instructions for CE's ASSEMBLE | Done | Sep | Closed, no listing check outstanding |
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
| M6 |   | CMS | 31-bit CMS proper; larger minidisks (4K blocks) | Planned |  | Before M9 |
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
| M9 | 9.2 | CP leftovers | I-235 (models), I-201 (DMKDMP dump), warm start (I-267, I-268), LPSW fast path above 16 MB, per-user IP (9.12a) | In progress | 11 Oct | I-201, I-267, I-268 done: dump, SHUTDOWN and warm start work |
| M9 | 9.3 | C++ | C++ on CMS (subset, cross route) | Planned |  |  |
| M9 | 9.4 | Database | SQL: SQLite on CMS + SQLSERV; MariaDB/PostgreSQL on Debian | Planned |  |  |
| M9 | 9.5 | Network | CMS sockets + httpd | Planned |  |  |
| M9 | 9.6 | dbase | xBase / Harbour | Planned |  |  |
| M9 | 9.7 | MQ | MQ and MQTT (CMS and Debian) | Planned |  | mosquitto on Debian done |
| M9 | 9.8 | APL | APL in C on CMS | In progress |  | Debian 34/34; CMS run pending |
| M9 | 9.9 | REXX | ooRexx 5 on Debian | Planned |  |  |
| M9 | 9.10 | AI | AI gateway service machine (ASKSERV) | Planned |  |  |
| M9 | 9.11 | Snapshot | Kit build 7 (distributable snapshot) | Planned |  | After the list above |
| M9 | 9.12 | Firewall | Network pack: per-user addresses, CP guest LAN, BORDER firewall, inside DHCP/DNS (9.12a-d) | Planned |  | After M4b.3, before M9 |
| M9 | 9.12a | Addresses | Per-user IP address at disk IPL from the directory (fixes I-266) | Next |  | Done with 9.2 |
| M9 | 9.12b | Guest LAN | CP virtual NIC per guest on one internal segment (LCS/QETH style, like z/VM Guest LAN) | Planned |  | Needed for DHCP: CTC links are point-to-point |
| M9 | 9.12c | Firewall | BORDER service machine: only outside link; NAT outbound; port forwards to service machines (httpd 9.5) | Planned |  | Outside link may be bridged (LCS on tap) to get its address from the LAN DHCP |
| M9 | 9.12d | DHCP/DNS | DHCP and DNS for the inside segment as a service machine (dnsmasq on Debian, or in BORDER) | Planned |  | Needs 9.12b |
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
