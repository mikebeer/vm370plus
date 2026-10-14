#!/usr/bin/env python3
"""cckdzero.py -- zero the free blocks of a Linux ext2 disk in a CCKD image.

    python3 cckdzero.py in.cckd out.cckd [--check sparse.img]

A Linux disk (3390, CDL format, one partition from track 2, 4 KB blocks)
keeps whatever its free blocks held before: after a file system is copied
onto a reused disk, the old data still compresses into the .cckd (the
Debian disk moved to a 3390-9 in 9.12 grew from 270 MB to 476 MB that way).
This reads the ext2 block bitmaps and writes a new CCKD file in which
every free block is zero and every all-zero track is a null track, so it
costs nothing.  The used blocks and tracks 0-1 (labels, IPL records) are
copied unchanged.  Run it on a disk no Hercules has open.

--check writes the used blocks into a sparse image of the partition, for
`e2fsck -fn` on the PC (only the used blocks take space).

Hercules 3.13 CCKD format: a 512-byte CKD device header, a 512-byte CCKD
header, the level-1 table, then level-2 tables (256 entries of pos, len,
size) and track images (5-byte header, then the rest, zlib or bzip2
compressed per byte 0).  The new file has no free space.
"""
import struct, sys, zlib, bz2

HDR, CHDR = 512, 512
RECS, BLK = 12, 4096                # 3390, 4 KB blocks
PART0 = 2                           # the partition starts at track 2


def main():
    a = sys.argv[1:]
    check = None
    if '--check' in a:
        i = a.index('--check'); check = a[i + 1]; del a[i:i + 2]
    src, dst = a
    f = open(src, 'rb')
    devhdr = f.read(HDR)
    chdr = bytearray(f.read(CHDR))
    if devhdr[:8] != b'CKD_C370':
        sys.exit('not a CCKD base file: %r' % devhdr[:8])
    heads = struct.unpack('<I', devhdr[8:12])[0]
    opts = chdr[3]
    e = '>' if opts & 2 else '<'
    numl1, numl2 = struct.unpack(e + 'ii', chdr[4:12])
    cyls = struct.unpack('<I', chdr[40:44])[0]
    nullfmt, comp = chdr[44], chdr[45]
    ntrk = cyls * heads
    l1 = struct.unpack(e + '%dI' % numl1, f.read(4 * numl1))

    def l2tab(i):
        if l1[i] in (0, 0xFFFFFFFF):
            return [(0, 0, 0)] * 256
        f.seek(l1[i])
        d = f.read(8 * 256)
        return [struct.unpack(e + 'IHH', d[j * 8:j * 8 + 8]) for j in range(256)]

    l2cache = {}

    def l2ent(t):
        i = t // 256
        if i not in l2cache:
            l2cache.clear(); l2cache[i] = l2tab(i)
        return l2cache[i][t % 256]

    def rawtrack(t):
        """The uncompressed track image, or None for a null track (an
        L2 entry with pos 0 and len = the null format, 0-2; format 2 is
        twelve zero 4 KB records, a Linux track)."""
        pos, ln, sz = l2ent(t)
        if pos in (0, 0xFFFFFFFF) or ln <= 2:
            return None
        f.seek(pos)
        d = f.read(ln)
        c = d[0] & 3
        if c == 0:
            return d
        body = zlib.decompress(d[5:]) if c == 1 else bz2.decompress(d[5:])
        return bytes([0]) + d[1:5] + body

    def records(img):
        """(record number, data offset, data length) of each record past R0."""
        p = 5 + 8 + 8                     # track header, R0 count and data
        out = []
        while img[p:p + 8] != b'\xff' * 8:
            c, h, r, kl, dl = struct.unpack('>HHBBH', img[p:p + 8])
            out.append((r, p + 8 + kl, dl))
            p += 8 + kl + dl
        return out

    def block(n):
        """Partition block n (4 KB), zeros if its track is null."""
        t, r = divmod(n + PART0 * RECS, RECS)
        img = rawtrack(t)
        if img is None:
            return bytes(BLK)
        for rn, off, dl in records(img):
            if rn == r + 1:
                return img[off:off + dl]
        raise ValueError('block %d: record %d not on track %d' % (n, r + 1, t))

    sb = block(0)[1024:2048]
    (magic,) = struct.unpack('<H', sb[56:58])
    if magic != 0xEF53:
        sys.exit('no ext2 superblock on the partition')
    nblocks, = struct.unpack('<I', sb[4:8])
    logbs, = struct.unpack('<I', sb[24:28])
    bpg, = struct.unpack('<I', sb[32:36])
    first, = struct.unpack('<I', sb[20:24])
    if 1024 << logbs != BLK:
        sys.exit('block size %d, not 4096' % (1024 << logbs))
    ngroups = (nblocks - first + bpg - 1) // bpg
    gdt = b''
    gb = first + 1
    while len(gdt) < ngroups * 32:
        gdt += block(gb); gb += 1
    used = bytearray((nblocks + 7) // 8)
    for g in range(ngroups):
        bb, = struct.unpack('<I', gdt[g * 32:g * 32 + 4])
        bm = block(bb)
        n = min(bpg, nblocks - g * bpg)
        start = g * bpg + first
        for i in range(n):
            if bm[i >> 3] >> (i & 7) & 1:
                b = start + i
                used[b >> 3] |= 1 << (b & 7)
    for b in range(first):                 # boot block area
        used[b >> 3] |= 1 << (b & 7)
    isused = lambda b: b >= nblocks or used[b >> 3] >> (b & 7) & 1
    nused = sum(bin(x).count('1') for x in used)
    print('ext2: %d blocks, %d used, %d groups' % (nblocks, nused, ngroups))

    chk = open(check, 'wb') if check else None
    if chk:
        chk.truncate(nblocks * BLK)

    o = open(dst, 'wb')
    o.write(devhdr)
    o.write(bytes(CHDR))                   # rewritten at the end
    l1pos = HDR + CHDR
    o.write(bytes(4 * numl1))
    newl1 = [0] * numl1
    zeroed = nulled = 0
    pos = l1pos + 4 * numl1
    for i in range(numl1):
        ents, imgs = [], []
        if l1[i] in (0, 0xFFFFFFFF):        # a null table stays null
            newl1[i] = l1[i]
            continue
        for j in range(256):
            t = i * 256 + j
            img = rawtrack(t) if t < ntrk else None
            if img is None:
                ents.append(l2ent(t) if t < ntrk else (0, nullfmt, nullfmt))
                imgs.append(None); continue
            if img is not None and t >= PART0:
                img = bytearray(img)
                allz = True
                for rn, off, dl in records(img):
                    b = (t - PART0) * RECS + rn - 1
                    if not isused(b):
                        if any(img[off:off + dl]):
                            img[off:off + dl] = bytes(dl); zeroed += 1
                    elif chk and b < nblocks:
                        chk.seek(b * BLK); chk.write(img[off:off + dl])
                    if any(img[off:off + dl]):
                        allz = False
                recs = records(img)
                if allz and len(recs) == RECS and \
                        all(dl == BLK for _, _, dl in recs):
                    ents.append((0, 2, 2)); imgs.append(None); nulled += 1
                    continue
            c = img[0] & 3 or comp & 3
            body = bytes(img[5:])
            if c == 1:
                z = zlib.compress(body, 6)
            elif c == 2:
                z = bz2.compress(body, 9)
            else:
                z = body
            if c and len(z) >= len(body):
                c, z = 0, body
            data = bytes([c]) + bytes(img[1:5]) + z
            ents.append(len(data)); imgs.append(data)
        newl1[i] = pos
        tabpos = pos
        pos += 8 * 256
        tab = []
        for x in ents:
            if isinstance(x, tuple):
                tab.append(x)              # a null track
            else:
                tab.append((pos, x, x)); pos += x
        o.write(b''.join(struct.pack(e + 'IHH', *x) for x in tab))
        for d in imgs:
            if d is not None:
                o.write(d)
        assert o.tell() == pos
    size = pos
    o.seek(l1pos)
    o.write(struct.pack(e + '%dI' % numl1, *newl1))
    chdr[3] = opts & ~(32 | 64 | 128)      # no space errors, closed
    struct.pack_into(e + 'IIIIIIi', chdr, 12, size, size, 0, 0, 0, 0, 0)
    struct.pack_into(e + 'I', chdr, 36, 0)
    o.seek(HDR)
    o.write(chdr)
    o.close()
    print('%d free blocks zeroed, %d tracks now null; %s: %d bytes'
          % (zeroed, nulled, dst, size))


if __name__ == '__main__':
    main()
