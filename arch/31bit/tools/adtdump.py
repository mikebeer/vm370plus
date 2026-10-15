#!/usr/bin/env python3
"""adtdump.py core.bin [MODE...] -- CMS active disk tables (ADT) and their
FST hyperblocks, read from a Hercules savecore image (I-297).

Finds every ADT by its shape (a 6-byte EBCDIC label, a mode letter A-Z at
+72) and prints the fields DMSLFS's lookups depend on: ADTFLG1-3, ADTFTYP,
ADTFDA, ADTHBCT, ADTFSTC, ADTCHBA/ADTCFST, ADTLHBA/ADTLFST, ADTQQM; then
walks the hyperblock chain (entries at ADTFDA+8, then +800 forward
pointers) and lists the FSTs (name, type, FSTFCL), checking the order the
sorted (ADTFSORT) binary search assumes."""
import struct, sys
E = bytes.maketrans(bytes(range(256)), bytes(range(256)))
def asc(b):
    return b.decode('cp037', 'replace')
core = open(sys.argv[1], 'rb').read()
modes = set(sys.argv[2:])
def w(a): return struct.unpack('>I', core[a:a + 4])[0]
def h(a): return struct.unpack('>H', core[a:a + 2])[0]
found = []
for a in range(0, min(len(core), 0x1000000) - 120, 4):
    m = core[a + 72]
    if not (0xC1 <= m <= 0xC9 or 0xD1 <= m <= 0xD9 or 0xE2 <= m <= 0xE9):
        continue
    lab = core[a:a + 6]
    if not all(0xC1 <= c <= 0xE9 or 0xF0 <= c <= 0xF9 or c == 0x40 for c in lab):
        continue
    fda, hbct, fstc = w(a + 20), w(a + 32), w(a + 36)
    flg1 = core[a + 74]
    if not (flg1 & 0x60) or fda == 0 or fda >= len(core) or hbct > 5000 or fstc > 100000:
        continue
    if w(fda) != 40 or w(fda + 4) != 800:
        continue
    found.append(a)
for a in found:
    m = asc(core[a + 72:a + 73])
    if modes and m not in modes:
        continue
    print('ADT %08X  %s (%s)  FLG1 %02X FLG2 %02X FLG3 %02X FTYP %02X  FDA %08X HBCT %d FSTC %d'
          % (a, m, asc(core[a:a + 6]), core[a + 74], core[a + 75], core[a + 6], core[a + 7],
             w(a + 20), w(a + 32), w(a + 36)))
    print('    CHBA %08X CFST %08X  LHBA %08X LFST %08X  QQM %08X  NACW %d RES %d'
          % (w(a + 40), w(a + 44), w(a + 96), w(a + 100), w(a + 80), h(a + 104), h(a + 106)))
    blk = w(a + 20) + 8; n = 0; prev = None; bad = 0; k = 0
    while blk and blk < len(core) and k < 5000:
        k += 1
        names = []
        for i in range(20):
            e = blk + 40 * i
            if core[e:e + 8] == bytes(8):
                names.append('-'); continue
            nm = asc(core[e:e + 8]).rstrip() + ' ' + asc(core[e + 8:e + 16]).rstrip()
            key = core[e:e + 16]
            if prev is not None and key < prev: bad += 1
            prev = key
            names.append('%s(%d)' % (nm, h(e + 28)))
            n += 1
        print('    HB %08X: %s' % (blk, ', '.join(names)))
        blk = w(blk + 800)
    print('    %d FSTs in %d hyperblocks, %d out of order' % (n, k, bad))
