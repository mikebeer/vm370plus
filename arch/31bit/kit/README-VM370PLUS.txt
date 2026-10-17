VM/370plus  --  overlay kit, 17 October 2026 (build 31)
========================================================

NEW IN BUILD 31
  - TICKET (19E): a helpdesk ticket from CMS.
        TICKET subject              then the description, an empty line
                                    ends it
        TICKET subject (FILE fn ft fm
    It reaches MChelpdesk (tenant vm370plus) as a mail from
    userid@vm370plus.lan; the answer names the ticket (VM3-000001).
    TICKET [VM3-000001] text adds to that ticket.  Tenant administrator:
    vm370plus / maint, password VM370plus-Maint-1 (change it).
  - The workers of MCproject and MChelpdesk run every 5 minutes (cron).


NEW IN BUILD 30
  - MariaDB 10.11 in the Debian guest (9.47), started at boot.  From CMS:
        MARIADB statement          or   MARIADB (FILE fn [ft [fm]]
    (19E; every CMS user gets an own account and database).
  - Our own applications in the Debian guest, on MariaDB and Regina 3.9:
      MCproject  (project management)  http://10.1.1.2:8080/
          operator admin@vm370plus.lan, password vm370plus-op1
      MChelpdesk (helpdesk)            http://10.1.1.2:8081/
          tenant system, login root, password Helpdesk-Root-Pw1
    Change these passwords.  A login takes about a minute on the 31-bit
    guest.  The network to the PC must be on (CTC line in vm370ce.conf).
  - Debian has perl, curl and Regina 3.9.5 (/usr/local/regina39) now.


NEW IN BUILD 29  (fixes from an early user's report)
  - MAINT has the Debian disk at logon (DEDICATE 250 190): LINUX no
    longer stops with DEV 250 DOES NOT EXIST.
  - LINUX EXEC attaches what Debian needs: the Debian disk 190 as 250 and
    the CTC pair 600/601 as 620/630 (class B users such as MAINT; others
    are told what to ask the operator for), and says when the machine has
    less than 64 MB:  cp def stor 64m, then linux.
  - LANG under the 190 CMS (IPL CMS) explained: the language disk is in
    the EDF format, which only the EC-mode CMS reads.  LANG now says so
    and shows the four commands (cp def stor 256m, cp link maint 290 290
    rr, ipl 290, lang).
  - The CTC line in vm370ce.conf is still commented out in the kit (the
    PC side needs setting up first); the setup wizard switches it on.

NEW IN BUILD 28
  - ZORK (9.13e): Dungeon, the MIT mainframe adventure, on the language
    disk (LANG, then ZORK; SAVE writes DSAVE DAT A).
  - The setup wizard offers to install what is missing (asking first):
    Hercules -- on Linux by building the newest SDL Hyperion with
    hercules-helper, or Homebrew, or the distribution's package; on macOS
    with Homebrew -- and a tn3270 emulator (c3270/x3270).


NEW IN BUILD 27
  - The setup wizard (9.45): vm370plus-setup.py, beside the kit parts and
    in vm370plus/tools/.  See INSTALL WITH THE WIZARD below.  The Debian
    guest has the spool-bridge service setup that takes its settings.


NEW IN BUILD 26
  - NJE (9.13b): VM/370plus is NJE node VM370CE.  NJE-II in the Debian
    guest speaks TCPNJE (port 175) to RSCS, JES2 or NJE-II peers; the
    first peer is PCNJE at 10.1.1.1 (NJE-II on the PC, see
    vm370plus/linux/nje/README.md).  On CMS: SENDFILE fn ft fm TO user
    AT node and TELL user AT node message; files from other nodes come
    to the reader (NETDATA: RECEIVE).  RSCS is the gateway machine
    (NJEGATE EXEC, started by AUTOLOG1); CE's RSCS V1 is RSCSV1.
    Peers: njeconf in the Debian guest, then /etc/init.d/nje restart.


NEW IN BUILD 25
  - EXEC 2 (9.43): an EXEC whose first line starts with &TRACE runs under
    EXEC2 EXEC (19E, our own interpreter) when CMS is IPLed from 290;
    EXEC2 fn args anywhere.
  - MAINT's PROFILE EXEC (ACCESS 5E5 B/B) is back (b22-b24 lacked it).
  - The llama model (stories15M, 58 MB) is fetched, not shipped: Debian
    llama --get (from vm370plus.com, /etc/llama.conf); CMS LLAMA GET fm
    (through the Debian guest; fm an EDF disk with 60 MB free; about an
    hour of punching).  vmspoold's outbox takes FILE path (binary).


NEW IN BUILD 24
  - ASK question [(FILE fn ft fm]: the AI gateway (9.10).  The Debian
    guest passes the question to the model in /etc/vmspool.d/ask.conf:
    llama.cpp's llama-server or Ollama on the PC (10.1.1.1), any
    OpenAI-compatible service, or Anthropic's API.  ASK NEW forgets the
    earlier questions; ask on Linux does the same.
  - vmprint (vm370plus/tools/vmprint.py): CP's printer 10E (class P, a
    socket printer, port 1403) on the PC; each print file becomes a .txt
    or .ps (--pdf, --lp queue).  On CMS: PCPRINT fn ft fm.
  - LLAMA (9.17): Karpathy's llama2.c on CMS (language disk: LLAMA prompt
    (MODEL fn ft TEMP t STEPS n) and in Debian (llama prompt).  The model
    is not included: stories15M.bin from huggingface.co/karpathy/tinyllamas
    (Debian: /usr/local/share/llama2c/; CMS: STORIES BIN on an EDF disk).


NEW IN BUILD 23
  - BORDER answers for the LAN's own names (9.40): in BORDER CONFIG
    (MAINT 19E),  DOMAIN name  (default vm370plus.lan),  HOST name address,
    MX preference name.  ping maint, ssh cmsuser.vm370plus.lan; other
    names go on to the DNS server as before.  Kernel #13
    (vm370plus/linux/linux13.rdr) on BORDER's 191.
  - Mail (9.18): userid@vm370plus.lan.  SENDMAIL fn [NOTE] sends a note
    (To:, Cc:, Bcc:, Subject:, an empty line, the text); GETMAIL reads the
    MAIL files in your reader into MAILnnnn MAIL A.  The Debian guest
    (MAINT) is the mail host: vmmaild on port 25; set SMARTHOST in
    /etc/vmspool.d/mail.conf for mail to the internet.


NEW IN BUILD 22
  - vmspool (vm370plus/tools/vmspool.py, Python 3, Windows or Linux):
    a file on the PC straight into a VM/370plus user's reader, e.g.
        python vmspool.py hello.exec cmsuser
        python vmspool.py prog.text cmsuser --binary
    then on CMS: READCARD HELLO EXEC A (or RDRLIST).  Two socket card
    readers in vm370ce.conf take the decks (010C, port 3505, text;
    020C, port 3506, binary); AUTOLOG1 starts them at IPL.
    A text file with lines over 80 characters goes as a stream:
    READCARD fn ft A, then UNSTREAM fn ft (19E).
  - MDPRINT fn [ft [fm]] (TYPE|DISK|PRINT|PS WIDTH n: Markdown documents
    printed on CMS (headings, lists, code, quotes, tables); (PS sends
    them to Debian (md2ms and groff) for PostScript, then PSPRINT.
    In Debian: mdprint FILE [-w N | -ps | -pdf].


NEW IN BUILD 21
  - An EXEC can name its interpreter on its first line:  /*!REGINA */
    (still a REXX comment) runs it with Regina instead of BREXX.
    SETREXX REGINA makes Regina the interpreter for every EXEC that
    starts with a plain /* (until the next IPL of CMS); SETREXX BREXX
    goes back, SETREXX alone shows the setting.  (IPL 290, after LANG.)
  - Regina is resident (XLRES): Regina EXECs may call each other, and
    Regina works with CMS as BREXX does -- PUSH, QUEUE, PULL and
    QUEUED() use the CMS console stack, EXECIO ... (STEM name. / (VAR
    name and GLOBALV GET / PUT reach Regina's variables, and PARSE
    SOURCE gives  CMS COMMAND fn ft * fn CMS.
  - XLRES LOAD name keeps any large program (fn XIMAGE) resident above
    16 MB as a command; XLRES alone lists them.


IN BUILD 20
  - cREXX built ON CMS (M8.2): RXC82, RXAS82 and RXBVM82 on the
    language disk are the current cREXX compiler, assembler and VM,
    compiled on CMS by the GCC 13 that runs there.  Use them as the
    M8.1 tools (Guide, Chapter 7):  RXC82 -L A fn,  RXAS82 -L A fn,
    RXBVM82 -L A fn.  CRX82 EXEC rebuilds them on CMS (DEFINE
    STORAGE 256M, IPL 290; a few hours).
  - Regina REXX 3.9.5 on CMS:  REGINA fn.ft  runs a REXX program from
    a CMS file (language disk; IPL 290).  Commands go to CMS, CP
    commands to CP, files are CMS files.  Try:  REGINA RT.REX
  - ooRexx 5.3 in the Debian guest:  oorexx prog.rex  (and oorexxc);
    rexx  stays Regina 3.6.
  - CP DUMP works again (it stopped CP, I-298), above 16 MB too.
  - A newly FORMATted EDF disk no longer damages the file directory of
    another accessed EDF disk ("File not found" on the language disk
    after writing to the new one, DMSFRE161T on RELEASE; I-297).
  - GCC 13 on CMS (CC1, XL) uses storage better: larger programs
    compile in 256M.


IN BUILD 19
  - DHCP and DNS on the guest LAN (BORDER): a Linux guest without an
    address of its own gets one (10.3.0.100-199) with BORDER as router
    and name server; BORDER passes name lookups on to the PC
    (10.1.4.1) or to the server in BORDER CONFIG:   DNS 8.8.8.8


IN BUILD 18
  - BORDER (Guide, Chapter 5): the guest LAN's way out to the PC.  A
    service machine running a small Linux with NAT and a firewall:
    its LAN address is 10.3.0.1, its CTC to the PC 10.1.4.2 (pair
    0606/0607; remove the # in front of  0606.2 CTCI  in vm370ce.conf).
    Start it from OPERATOR:   autolog border border
    BORDER CONFIG on MAINT 19E says which ports of 10.1.4.2 lead to
    which guest:   EXPOSE TCP 2223 TO 10.3.0.3 22   (then from the PC:
    ssh -p 2223 root@10.1.4.2).  Everything else is closed.  Guests
    without a CTC use BORDER as their default route.
  - Kernel #12 (vm370plus/linux/linux12.rdr): #11 plus netfilter.


IN BUILD 17
  - THE GUEST LAN (Guide, Chapter 5): Linux guests talk to each other
    over a LAN that CP simulates, like the z/VM guest LAN.  Before
    starting Linux:
        cp define lcs 640
        cp define lcs 641
        cp ipl linux
    Debian brings eth0 up by itself (vm370plus-lan): MAINT 10.3.0.2,
    CMSUSER 10.3.0.3, others in /etc/vm370plus-lan.conf.  Every
    adapter is on the one segment 10.3.0.0/24; there is no way out of
    it yet (the BORDER firewall comes next).  The Debian disk has the
    new kernel (#11, with the LCS driver); vm370plus/linux/linux11.rdr
    is the same kernel as a reader deck.
  - BOOKIE: the paragraph after a figure is no longer bold, no word is
    printed below the page foot, table text is black.


IN BUILD 16
  - Java and NetRexx on CMS (language disk, after LANG; IPL 290 and
    DEFINE STORAGE 128M or more):  JAVA class  runs fn CLASS (JamVM 2.0,
    GNU Classpath 0.99),  JAVAC fn  compiles fn JAVA (Eclipse compiler
    ecj 3.5.2),  NETREXX fn  runs fn NRX (NetRexx 3.05) and
    NETREXX fn COMPILE  makes fn CLASS.  Try JAVA HELLO, NETREXX HELLO.
  - Java and NetRexx in the Debian guest: java, javac (ecj), nrc
    (NetRexx compiler), nr (run a .nrx).  Give Linux 192 MB for them:
    MAINT may now DEFINE STORAGE up to 256M (was 64M).
  - CP no longer stops (PGS001/PTR018) when one user pages more than
    128 MB (I-290).  Large programs started with XL get more storage, and
    no longer print "Invalid character in fileID" (I-289).


IN BUILD 15
  - Debian keeps running when its terminal goes away (I-288).  Closing the
    window or losing the connection used to stop the whole guest (no ssh,
    no spool bridge) and log it off after 15 minutes.  Now it is just
    disconnected, as under z/VM.  To get the console back: from OPERATOR
    VARY ONLINE 00A and ENABLE 00A (with your terminal connected), then
    LOGON MAINT and BEGIN.  CMS users keep VM/370's rule.


IN BUILD 14
  - The Debian disk (LNX190) is a full 3390-9 again: 6.8 GB, 5.8 GB free
    (it was 97% full).  Its free blocks are zeroed, so it downloads as
    245 MB.  Join it with JOINDEB.CMD / joindeb.sh as before.
  - PostgreSQL 16 in the Debian guest, started at boot.  From CMS (lang):
        PGSQL create table t (a int, b text)
        PGSQL select * from t
        PGSQL (FILE QUERY SQL       the statements of QUERY SQL A
    Each CMS user gets a database of its own.  In Debian: psql -U postgres.
  - MQ: message queues for CMS programs on a RabbitMQ broker (IPL 290,
    lang, NICOF proxies running): MQ PUT|GET|DEPTH|PURGE queue [message].
    MQ OPTIONS names the broker (default: the PC, guest/guest).
  - RXPIP: the Debian side follows the current rexxpository client
    (no update check, private packages through a cmsbridge token).
  - CP: a Linux guest with a CTC link no longer stays in the dispatch
    queue while idle, so two Linux guests share the machine properly; a
    virtual CTCA accepts guest buffers above 16 MB (I-287, in progress:
    Linux-to-Linux virtual CTCAs do not connect yet).


IN BUILD 13
  - NICOF for the EC-mode CMS (IPL 290), built with GCC 13, on the language
    disk (lang first; the NICOF proxies running: nicof.sh start):
        CMSFTP31 -p 2121           FTP server for your disks
        NHFS31 LIST|GET|PUT ...    files in vm370plus/nicof/nhfs/<user>
        HACC31 ...                 any PC file (enable service.4 first)
        IPLOOK31 -ME               name lookup through the PC
        HTTPD31 -p 8080 [-k key]   a web server for a CMS disk
        NCTEST                     socket test (echo server on PC port 8200)
  - I-285: NICOF programs no longer wait forever under IPL 290.
  - I-286: HX under IPL 290 no longer ends in a disabled wait.


IN BUILD 12
  - CP/M 2.2 (Guide, Chapter 8), through the RunCPM Z80 emulator:
        lang, then (IPL 290, more than 16 MB)
        CPM                        CP/M at the terminal; EXIT ends it
        CPM cmd [; cmd ...]        CP/M commands, then back to CMS
        CPM IMPORT fn ft           CMS text -> CP/M text (EXPORT: back)
    CP/M drive X: is CMS disk X; L: (the language disk) has PIP, STAT, ASM,
    LOAD, DDT, ED ...  Try: COPYFILE HELLO ASM L = = A, CPM IMPORT HELLO ASM,
    CPM L:ASM HELLO; L:LOAD HELLO; HELLO
    In the Debian guest: runcpm (Linux users), and CPM ... (LINUX from CMS.


IN BUILD 11
  - TELL to the internet:  TELL user AT NET message  publishes to the MQTT
    broker of the Debian guest (topic vm/tell/out/user); outside users
    answer by publishing to vm/tell/from/<login>/to/<userid>.  Logins in
    the guest:  tellnet-user name password.  On the PC:
        python3 vm370plus/tools/netfwd.py --tls vm.crt vm.key 8883 10.1.1.2:1883
    (the router forwards 8883 to the PC); vm370plus/tools/nettell.py is a
    small outside client (pip install paho-mqtt).
  - NICOF (Guide, Chapter 6): sockets for CMS programs, the FTP server
    CMSFTPD and NHFS file transfer to the PC.  Needs Java on the PC:
        sh vm370plus/nicof/nicof.sh start        (after Hercules is up)
    then on CMS:  NCFACC,  NHFS LIST,  CMSFTPD -p 2121 -usecmscmds
    (Windows: start the same java command as in nicof.sh in vm370plus\nicof)
  - I-283: REXX EXECs in a machine without OPTION ECMODE no longer end with
    an operation exception (CP simulates STNSM/STOSM in BC mode too).


IN BUILD 10
  - TELL (Guide, Chapter 6):
        TELL userid message            CP MSG to a VM/370 user
        TELL user AT LINUX [message]   to a user of the Debian guest, through
                                       the spool bridge (ALL: everybody);
                                       without a message it asks for one
    On Linux:  tell userid message  answers (CP MSG, with the Linux user's
    name in front);  tell  shows the messages that came while away.
  - I-277 fixed: a REXX EXEC with 8 or more arguments no longer ends CMS in
    machines below 15 MB.  CE's saved segment GCCLIB was older than its
    GCCLIB TEXT (argv room for 8 pointers); it has been saved again.
  - The bridge and document EXECs (LXCALL, PHONE, MQTT, TELL, RXPIP,
    TROFF, BOOKIE, LATEX, PSPRINT) are also on MAINT 19D: they work under
    the usual  ipl cms.


IN BUILD 9
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

INSTALL WITH THE WIZARD  (9.45, build 27)
  Put VM370CE_V1_R1_2.zip, all kit parts and vm370plus-setup.py in one
  directory and run there
        python3 vm370plus-setup.py        (Windows: py vm370plus-setup.py)
  It checks for Python and Hercules, unpacks CE and every part into a new
  directory, joins the Debian and language disks (SHA-256 checked), asks
  for the storage size, the 3270 port and the network link to the Debian
  guest and writes them into vm370ce.conf.  It also asks for the mail
  smarthost, the AI model server and the NJE peers; once Debian runs,
        python3 vm370plus-setup.py --apply <directory>
  sends those to the guest (through the PC card reader, port 3505).
  --yes takes the defaults, --answers file.json repeats an installation.

INSTALL BY HAND
  1. Unzip VM370CE_V1_R1_2.zip into a new directory.
  2. Unzip ALL kit parts OVER it, replacing files:
     part 1: config, system shadows, tools, README; part 2:
     disks/vm50-4.cckd; part 3: the CMSUSER volume's shadow; part 4:
     Linux files (vm370plus/linux/, the Guide) and the VM50-2 shadow
     (MAINT's 19D); part 5: the VM50-3 shadow; part 6: the VM50U1
     shadow (BORDER's disk); parts 7-16: the Debian
     disk in pieces -- then join it: JOINDEB.CMD (Windows) or
     sh joindeb.sh (Linux/WSL), see INSTALL THE DEBIAN DISK; parts
     17-19: the language disk -- join it: JOINLANG.CMD or sh joinlang.sh.
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
  3390-1 before), parts 7-16 of the kit, see INSTALL THE DEBIAN DISK.
  INSTALL THE DEBIAN DISK
    The disk is about 255 MB compressed, shipped in ten pieces (parts
    7-16).  After unzipping them into the CE directory, run there
        JOINDEB.CMD            (Windows cmd.exe)
        sh joindeb.sh          (Linux/WSL; also checks lnx190.sha256)
    which joins disks/lnx190.cckd.001-009 into disks/lnx190.cckd.
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
