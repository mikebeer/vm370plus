#!/usr/bin/env python3
"""shadowrevert.py SHADOW CYLS... [--out NEW] [--report]

Hercules CCKD shadow file: drop the tracks of the given cylinders
(ranges a-b) from the shadow, so they read from the base image again.

For the kit (I-272 follow-up, 16 Oct): paging and spool live in the TEMP
(and T-disk) cylinders of VM50-2/3/4; every test leaves page slots
behind, and the shadows grew by 70 MB in one day.  After all spool files
are purged and CP shut down, nothing points into those cylinders, so the
kit can carry the base's tracks instead.  --report lists how many tracks
of each range the shadow holds (and its size), changing nothing.

Format (Hercules 3.13): 512-byte device header (CKD_S370 for a shadow),
512-byte CCKD header (numl1 at +4, cyls at +40), the level-1 table, then
256-entry level-2 tables of (pos, len, size); in a shadow an L1 or L2
entry of all ones means "not in this file" (read the base), pos 0 a
null track.  The output is written fresh, without free
space; track images are copied as they are.
"""
import struct, sys
NOT = 0xFFFFFFFF
GONE = (NOT, 0xFFFF, 0xFFFF)

def main():
    a = sys.argv[1:]
    out = None; report = '--report' in a
    if report: a.remove('--report')
    if '--out' in a:
        i = a.index('--out'); out = a[i + 1]; del a[i:i + 2]
    src, rng = a[0], []
    for r in a[1:]:
        lo, _, hi = r.partition('-'); rng.append((int(lo), int(hi or lo)))
    f = open(src, 'rb')
    dev = f.read(512); ch = bytearray(f.read(512))
    if dev[:8] not in (b'CKD_S370', b'CKD_C370'):
        sys.exit('not a CCKD file: %r' % dev[:8])
    heads = struct.unpack('<I', dev[8:12])[0]
    e = '>' if ch[3] & 2 else '<'
    numl1 = struct.unpack(e + 'i', ch[4:8])[0]
    l1 = struct.unpack(e + '%dI' % numl1, f.read(4 * numl1))
    drop = lambda t: any(lo <= t // heads <= hi for lo, hi in rng)
    tabs = {}
    for i in range(numl1):
        if l1[i] == 0xFFFFFFFF: continue
        f.seek(l1[i]); d = f.read(2048)
        tabs[i] = [struct.unpack(e + 'IHH', d[j * 8:j * 8 + 8]) for j in range(256)]
    if report:
        for lo, hi in rng:
            n = b = 0
            for i, t in tabs.items():
                for j, (p, ln, sz) in enumerate(t):
                    if p != NOT and lo <= (i * 256 + j) // heads <= hi:
                        n += 1; b += ln if p else 0
            print('cyl %d-%d: %d tracks, %d bytes' % (lo, hi, n, b))
        return
    o = open(out, 'wb'); o.write(dev); o.write(bytes(512))
    pos = 1024 + 4 * numl1; o.write(bytes(4 * numl1)); newl1 = list(l1)
    dropped = 0
    for i in range(numl1):
        if i not in tabs: continue
        ents = []; imgs = []
        for j, (p, ln, sz) in enumerate(tabs[i]):
            if p != NOT and drop(i * 256 + j):
                ents.append(GONE); dropped += 1; continue
            if p in (0, NOT):
                ents.append((p, ln, sz)); continue
            f.seek(p); imgs.append(f.read(ln)); ents.append(None)
        if all(x == GONE for x in ents):
            newl1[i] = NOT; continue
        newl1[i] = pos; tp = pos; pos += 2048; tab = []; k = 0
        for x in ents:
            if x is None:
                d = imgs[k]; k += 1; tab.append((pos, len(d), len(d))); pos += len(d)
            else:
                tab.append(x)
        o.write(b''.join(struct.pack(e + 'IHH', *x) for x in tab))
        for d in imgs: o.write(d)
        assert o.tell() == pos
    o.seek(1024); o.write(struct.pack(e + '%dI' % numl1, *newl1))
    ch[3] &= ~(32 | 64 | 128) & 0xFF               # as cckdzero.py
    struct.pack_into(e + 'IIIIIIi', ch, 12, pos, pos, 0, 0, 0, 0, 0)
    struct.pack_into(e + 'I', ch, 36, 0)
    o.seek(512); o.write(ch); o.close()
    print('%s: %d tracks dropped, %d bytes' % (out, dropped, pos))

main()
