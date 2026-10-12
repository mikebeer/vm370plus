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

Headers and libraries: SYSROOT TAR and CXXROOT TAR (read as /usr/...).
Large programs (CC1, PY, XBASEC) run from above 16 MB through XL.
The VM/370plus Guide, chapter 8, describes each one.
