VM/370plus language disk (MAINT 19F, volume VMLANG)

  LANG            link MAINT 19F R/O and access it as L
  IPL 290         the EC-mode CMS, and CP DEFINE STORAGE 256M

  CC fn           C (fn C) or C++ (fn CPP/CC/CXX) with GCC 13 -> fn MODULE
  COBOL fn        GnuCOBOL 3.1.2: fn COB -> fn MODULE
  COBX fn         COBOL with OO (mikebeer/cobol): fn COB -> fn MODULE
  PLIC fn         PL/I (mikebeer/pli): fn PLI -> fn MODULE
  PYTHON fn       Python 3 (mikebeer/python, NumPy/pandas/SciPy built in)
  SQLITE fn.db    SQLite 3.46 shell;  SQL db stmt  through SQLSERV
  PCPAS fn        Stanford Pascal 2026.10 to P-code;  PCRUN fn  runs it
  APL fn.apl      APL (mikebeer/apl)
  XBASE fn.prg    xBase/Clipper (mikebeer/xbase)
  SMALLTLK fn.st  Smalltalk-80 (mikebeer/smalltalk)
  ZIP / UNZIP     Info-ZIP zip 3.0 and unzip 6.0 (.zip as on the PC)
  GZIP [-d] fn.ft gzip format: fn ft -> fn ft-GZ (zlib minigzip)
  UNTERSE in out  IBM TERSE (PACK/SPACK) files from z/OS; -b binary
  TERSE in out    compress to TERSE SPACK (-v V records, -b binary)
  CPM             CP/M 2.2 (RunCPM Z80 emulator): drive X: is CMS disk X,
                  L: has PIP, STAT, ASM, LOAD, DDT, ED; CPM IMPORT fn ft
                  makes a CMS text file a CP/M one (EXPORT: back)
  NHFS31 ...      NICOF host files (GCC 13): LIST, GET, PUT, TYPE, MKDIR in
                  your area of the PC's nhfs directory; HACC31 any PC file
  CMSFTP31 -p n   an FTP server for your disks on the PC's port n (NICOF)
  IPLOOK31 name   name or address lookup through the PC (NICOF)
  HTTPD31         a web server for a CMS disk (-p port -d fm -n count -k key)
  MQ PUT|GET|DEPTH|PURGE queue [msg]  message queues on a RabbitMQ
                  broker through NICOF (MQ OPTIONS: HOST PORT USER ...)
  NCTEST          a NICOF socket test against an echo server on the PC
                  (NICOF's proxies: nicof/nicof.sh start on the PC)

Headers and libraries: SYSROOT TAR and CXXROOT TAR (read as /usr/...).
Large programs (CC1, PY, XBASEC) run from above 16 MB through XL.
The VM/370plus Guide, chapter 8, describes each one.
