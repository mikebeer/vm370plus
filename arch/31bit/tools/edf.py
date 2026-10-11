#!/usr/bin/env python3
"""edf.py -- CMS EDF (Enhanced Disk Format) minidisks on the PC side (M6).

The reference implementation for VM/370plus's EDF support in CMS: it formats,
lists, reads and writes EDF disks held in Hercules CKD images, so what CMS
writes can be checked byte for byte, and files can be put on a CMS disk from
the PC.  The layout is IBM's (VM/SP and later CMS), as Linux's cmsfs-fuse
(s390-tools, MIT licence) reads it:

  block n (1-origin) = record (n-1) % bpt + 1 on track (n-1) // bpt
  blocks 1-2   IPL records          block 3  label 'CMS1'
  block DOP    first block of the directory (DOP = 4)
  label        'CMS1' volid(6) ver(2) blksize dop fcyl maxcyl total used
               fstsize(64) fst/blk date(6) ...           (fullwords, offset 12)
  FST (64)     fn ft datew timew wrpnt rdpnt fmode reccnt fclpt recfm flags
               lrecl blkcnt yearw | FOP ADBC AIC NLVL PTRSZ ADATI(6) res(4)
  directory    a file of FSTs: its own (X'0000000100000000' DIRECTOR),
               then ALLOCMAP (X'0000000200000000'), then the files
  ALLOCMAP     one bit per block, MSB first, block n = bit n-1, 1 = used
  F files      level 0: FOP = the data block; level k: pointer blocks of
               4-byte block numbers (0 = a null block of zeros)
  V files      records = 2-byte length + data, packed across blocks; pointer
               entries of 12 bytes (block, highest record number, disp =
               bytes of the previous record at the start of the block, or
               X'FFFFFFFF' when the whole block continues one); the last
               4 bytes of a V pointer block = offset of its last entry

Images: Hercules compressed (.cckd) images are converted with cckd2ckd /
ckd2cckd around the work (`--cckd`); a plain .ckd is used in place.

usage: edf.py IMAGE format VOLID [BLKSIZE]
       edf.py IMAGE list
       edf.py IMAGE get FN FT [OUT]        (EBCDIC records -> text lines)
       edf.py IMAGE put FN FT FM IN [F lrecl|V]
       edf.py IMAGE erase FN FT
       edf.py IMAGE label
"""
import os, struct, subprocess, sys, tempfile, time

E2A = bytes(range(256)).decode('cp037').encode('latin-1', 'replace')

def ebc(s, n=None):
    b = s.encode('cp037')
    return b if n is None else b.ljust(n, b'\x40')[:n]

def asc(b):
    return b.decode('cp037')

# 4096-byte blocks per track (CP's own paging tables: DMKCKP HD3350 etc.)
BPT = {0x50: {4096: 4}, 0x75: {4096: 8}, 0x80: {4096: 10}, 0x90: {4096: 12}}
DEVNAME = {0x50: '3350', 0x75: '3375', 0x80: '3380', 0x90: '3390'}


class CKD:
    """A plain (uncompressed) Hercules CKD image."""
    def __init__(self, path):
        self.path = path
        self.f = open(path, 'r+b')
        h = self.f.read(512)
        if h[:8] not in (b'CKD_P370', b'CKD_C370'):
            raise SystemExit('edf: %s is not a CKD image' % path)
        if h[:8] == b'CKD_C370':
            raise SystemExit('edf: %s is compressed; use --cckd' % path)
        self.heads, self.trksize = struct.unpack('<II', h[8:16])
        self.devtype = h[16]
        hc = struct.unpack('<H', h[18:20])[0]
        size = os.path.getsize(path)
        self.ntrk = (size - 512) // self.trksize
        self.ncyl = self.ntrk // self.heads

    def track(self, t):
        self.f.seek(512 + t * self.trksize)
        return bytearray(self.f.read(self.trksize))

    def put_track(self, t, data):
        assert len(data) == self.trksize
        self.f.seek(512 + t * self.trksize)
        self.f.write(data)

    def records(self, t):
        """[(r, kl, dl, offset of data)] of track t."""
        d = self.track(t)
        p, out = 5, []
        while d[p:p + 8] != b'\xff' * 8 and p + 8 <= len(d):
            cc, hh, r, kl, dl = struct.unpack('>HHBBH', d[p:p + 8])
            if r:
                out.append((r, kl, dl, p + 8 + kl))
            p += 8 + kl + dl
        return d, out

    def format_track(self, t, nrec, dl):
        cc, hh = divmod(t, self.heads)
        d = bytearray(b'\x00') + struct.pack('>HH', cc, hh)
        d += struct.pack('>HHBBH', cc, hh, 0, 0, 8) + bytes(8)     # R0
        for r in range(1, nrec + 1):
            d += struct.pack('>HHBBH', cc, hh, r, 0, dl) + b'\x00' * dl
        d += b'\xff' * 8
        if len(d) > self.trksize:
            raise SystemExit('edf: %d x %d does not fit a track' % (nrec, dl))
        self.put_track(t, bytes(d.ljust(self.trksize, b'\x00')))

    def close(self):
        self.f.close()


class EDF:
    def __init__(self, ckd):
        self.ckd = ckd
        self.bs = None
        self.bpt = None
        self._cache = {}

    # ---- blocks ----
    def locate(self, n):
        t, r = divmod(n - 1, self.bpt)
        return t, r + 1

    def read(self, n):
        t, r = self.locate(n)
        d, recs = self.ckd.records(t)
        for rr, kl, dl, off in recs:
            if rr == r:
                return bytes(d[off:off + dl])
        raise SystemExit('edf: block %d (track %d record %d) missing' % (n, t, r))

    def write(self, n, data):
        assert len(data) == self.bs, (n, len(data))
        t, r = self.locate(n)
        d, recs = self.ckd.records(t)
        for rr, kl, dl, off in recs:
            if rr == r:
                d[off:off + dl] = data
                self.ckd.put_track(t, bytes(d))
                return
        raise SystemExit('edf: block %d missing' % n)

    # ---- label ----
    def open(self):
        d, recs = self.ckd.records(0)
        if len(recs) < 3:
            raise SystemExit('edf: no CMS label (track 0 has %d records)' % len(recs))
        r, kl, dl, off = recs[2]
        lab = bytes(d[off:off + dl])
        if lab[:4] != ebc('CMS1'):
            raise SystemExit('edf: not an EDF disk (label %r)' % asc(lab[:4]))
        self.bs = dl
        self.bpt = len(recs)
        (self.blksize, self.dop, self.fcyl, self.maxcyl, self.total,
         self.used, self.fstsize, self.fstper) = struct.unpack('>8I', lab[12:44])
        self.volid = asc(lab[4:10])
        assert self.blksize == self.bs and self.fstsize == 64
        self.label = bytearray(lab)
        return self

    def write_label(self):
        struct.pack_into('>I', self.label, 32, self.used)
        self.write(3, bytes(self.label))

    # ---- pointer trees ----
    def nptr(self, ptrsz):
        return (self.bs - (4 if ptrsz == 12 else 0)) // ptrsz

    def data_blocks(self, fst):
        """[(blockno or 0, disp)] in order (disp only for V files)."""
        fop, adbc, levels, psz = fst['fop'], fst['adbc'], fst['levels'], fst['ptrsz']
        if adbc == 0:
            return []
        if levels == 0:
            return [(fop, 0)]
        out = []
        def walk(b, lvl):
            blk = self.read(b) if b else bytes(self.bs)
            per = self.nptr(psz)
            for i in range(per):
                if len(out) >= adbc:
                    return
                e = blk[i * psz:(i + 1) * psz]
                nxt = struct.unpack('>I', e[:4])[0]
                if lvl == 1:
                    disp = struct.unpack('>I', e[8:12])[0] if psz == 12 else 0
                    if psz == 12 and nxt == 0 and struct.unpack('>I', e[4:8])[0] == 0:
                        return
                    out.append((nxt, disp))
                else:
                    if psz == 12 and nxt == 0:
                        return
                    walk(nxt, lvl - 1)
        walk(fop, levels)
        return out[:adbc]

    def file_bytes(self, fst):
        out = bytearray()
        for b, disp in self.data_blocks(fst):
            out += self.read(b) if b else bytes(self.bs)
        return bytes(out)

    def records_of(self, fst):
        data = self.file_bytes(fst)
        n, recs = fst['aic'], []
        if fst['recfm'] == 'F':
            l = fst['lrecl']
            return [data[i * l:(i + 1) * l] for i in range(n)]
        p = 0
        while len(recs) < n and p + 2 <= len(data):
            l = struct.unpack('>H', data[p:p + 2])[0]
            if l == 0:
                break
            recs.append(data[p + 2:p + 2 + l])
            p += 2 + l
        return recs

    # ---- FSTs ----
    @staticmethod
    def unpack_fst(e):
        f = {}
        f['raw'] = e
        f['fn'], f['ft'] = e[0:8], e[8:16]
        f['fmode'] = asc(e[24:26])
        f['recfm'] = asc(e[30:31])
        f['flags'] = e[31]
        f['lrecl'] = struct.unpack('>I', e[32:36])[0]
        (f['fop'], f['adbc'], f['aic']) = struct.unpack('>III', e[40:52])
        f['levels'], f['ptrsz'] = e[52], e[53]
        f['date'] = e[54:60].hex()
        return f

    def directory(self):
        """[(slot index, fst)] for every entry, the first two included."""
        first = self.read(self.dop)
        me = self.unpack_fst(first[:64])
        assert me['ft'] == ebc('DIRECTOR'), me['ft']
        data = self.file_bytes(me)
        out = []
        for i in range(me['aic']):
            e = data[i * 64:(i + 1) * 64]
            out.append((i, self.unpack_fst(e)))
        self.dirfst = me
        return out

    def files(self):
        return [(i, f) for i, f in self.directory()[2:] if f['fn'] != bytes(8)]

    # ---- allocation ----
    def amap(self):
        d = self.directory()
        am = d[1][1]
        return bytearray(self.file_bytes(am)), am

    def put_amap(self, bits, am):
        for k, (b, _) in enumerate(self.data_blocks(am)):
            self.write(b, bytes(bits[k * self.bs:(k + 1) * self.bs]))

    @staticmethod
    def setbit(bits, n, on):
        i, m = (n - 1) // 8, 0x80 >> ((n - 1) % 8)
        bits[i] = (bits[i] | m) if on else (bits[i] & ~m)

    @staticmethod
    def getbit(bits, n):
        return bits[(n - 1) // 8] & (0x80 >> ((n - 1) % 8))

    def alloc(self, bits, k):
        got = []
        for n in range(1, self.total + 1):
            if not self.getbit(bits, n):
                self.setbit(bits, n, True)
                got.append(n)
                if len(got) == k:
                    return got
        raise SystemExit('edf: disk full')


def bcd(n):
    return ((n // 10) << 4) | (n % 10)

def fst_date():
    t = time.gmtime()
    return bytes(bcd(x) for x in (t.tm_year % 100, t.tm_mon, t.tm_mday,
                                  t.tm_hour, t.tm_min, t.tm_sec))

def make_fst(fn, ft, fm, recfm, lrecl, fop, adbc, aic, levels, ptrsz):
    t = time.gmtime()
    e = bytearray(64)
    e[0:8] = fn if isinstance(fn, bytes) else ebc(fn.upper(), 8)
    e[8:16] = ft if isinstance(ft, bytes) else ebc(ft.upper(), 8)
    struct.pack_into('>HH', e, 16, bcd(t.tm_mon) << 8 | bcd(t.tm_mday),
                     bcd(t.tm_hour) << 8 | bcd(t.tm_min))
    struct.pack_into('>HH', e, 20, min(aic + 1, 65535), 1)  # WP RP
    e[24:26] = ebc(fm.upper().ljust(2, '1'), 2)
    struct.pack_into('>HH', e, 26, min(aic, 65535), min(fop, 65535))
    e[30:31] = ebc(recfm)
    e[31] = 0x08                                   # century: 20xx
    struct.pack_into('>I', e, 32, lrecl)
    struct.pack_into('>H', e, 36, min(adbc, 65535))
    e[38:40] = ebc('%02d' % (t.tm_year % 100))
    struct.pack_into('>IIIBB', e, 40, fop, adbc, aic, levels, ptrsz)
    e[54:60] = fst_date()
    return bytes(e)


def build_tree(edf, bits, blocks, ptrsz, vinfo=None):
    """Pointer blocks over data `blocks`; return (fop, levels)."""
    if not blocks:
        return 0, 0
    if len(blocks) == 1:
        return blocks[0], 0
    per = edf.nptr(ptrsz)
    # entries at level 1: (block, hi record, disp)
    ents = [(b, (vinfo[i][0] if vinfo else 0), (vinfo[i][1] if vinfo else 0))
            for i, b in enumerate(blocks)]
    levels = 0
    while len(ents) > 1 or levels == 0:
        levels += 1
        nxt = []
        for g in range(0, len(ents), per):
            grp = ents[g:g + per]
            pb = edf.alloc(bits, 1)[0]
            blk = bytearray(edf.bs)
            for k, (b, hi, disp) in enumerate(grp):
                if ptrsz == 12:
                    struct.pack_into('>III', blk, k * 12, b, hi, disp)
                else:
                    struct.pack_into('>I', blk, k * 4, b)
            if ptrsz == 12:
                struct.pack_into('>I', blk, edf.bs - 4, (len(grp) - 1) * 12)
            edf.write(pb, bytes(blk))
            nxt.append((pb, grp[-1][1], grp[0][2]))
        ents = nxt
    return ents[0][0], levels


def write_file(edf, bits, recs, recfm, lrecl):
    """Data and pointer blocks for records; return (fop, adbc, aic, levels, psz)."""
    bs = edf.bs
    if recfm == 'F':
        data = b''.join(r.ljust(lrecl, b'\x40')[:lrecl] for r in recs)
        vinfo, psz = None, 4
    else:
        data, vinfo, psz = bytearray(), [], 12
        starts = []
        for r in recs:
            starts.append(len(data))
            data += struct.pack('>H', len(r)) + r
        data = bytes(data)
    nblk = (len(data) + bs - 1) // bs
    blocks = edf.alloc(bits, nblk) if nblk else []
    for i, b in enumerate(blocks):
        edf.write(b, data[i * bs:(i + 1) * bs].ljust(bs, b'\x00'))
    if recfm == 'V':
        # per block: highest record number starting in it (1-origin) and disp
        k = 0
        for i in range(nblk):
            lo, hi = i * bs, (i + 1) * bs
            while k < len(starts) and starts[k] < hi:
                k += 1
            # disp: bytes at the block start belonging to an earlier record
            prev = [s for s in starts if s < lo]
            if prev:
                ps = prev[-1]
                end = ps + 2 + len(recs[len(prev) - 1])
                disp = 0xFFFFFFFF if end > hi else max(0, end - lo)
            else:
                disp = 0
            vinfo.append((k, disp))
    fop, levels = build_tree(edf, bits, blocks, psz, vinfo)
    return fop, nblk, len(recs), levels, psz


def free_file(edf, bits, fst):
    def walk(b, lvl):
        if not b:
            return
        if lvl > 0:
            blk = edf.read(b)
            psz, per = fst['ptrsz'], edf.nptr(fst['ptrsz'])
            for i in range(per):
                n = struct.unpack('>I', blk[i * psz:i * psz + 4])[0]
                if n:
                    walk(n, lvl - 1)
        edf.setbit(bits, b, False)
    if fst['adbc']:
        walk(fst['fop'], fst['levels'])


def rewrite_directory(edf, entries, bits):
    """Rewrite the directory file from its FST list (entries[0] = itself)."""
    old = edf.dirfst
    # free the old directory blocks except DOP (it stays the first block)
    for b, _ in edf.data_blocks(old):
        if b != edf.dop:
            edf.setbit(bits, b, False)
    if old['levels']:
        def freeptr(b, lvl):
            blk = edf.read(b)
            for i in range(edf.nptr(4)):
                n = struct.unpack('>I', blk[i * 4:i * 4 + 4])[0]
                if n and lvl > 1:
                    freeptr(n, lvl - 1)
            edf.setbit(bits, b, False)
        freeptr(old['fop'], old['levels'])
    data = b''.join(entries)
    nblk = (len(data) + edf.bs - 1) // edf.bs
    blocks = [edf.dop] + (edf.alloc(bits, nblk - 1) if nblk > 1 else [])
    fop, levels = build_tree(edf, bits, blocks, 4)
    me = bytearray(entries[0])
    struct.pack_into('>IIIBB', me, 40, fop, nblk, len(entries), levels, 4)
    struct.pack_into('>H', me, 26, min(len(entries), 65535))
    data = bytes(me) + data[64:]
    for i, b in enumerate(blocks):
        edf.write(b, data[i * edf.bs:(i + 1) * edf.bs].ljust(edf.bs, b'\x00'))


def commit(edf, entries, bits, am):
    rewrite_directory(edf, entries, bits)
    edf.put_amap(bits, am)
    edf.used = sum(1 for n in range(1, edf.total + 1) if edf.getbit(bits, n))
    edf.write_label()


def do_format(ckd, volid, bs=4096):
    bpt = BPT.get(ckd.devtype, {}).get(bs)
    if not bpt:
        raise SystemExit('edf: no %d-byte blocks for device type %02X' % (bs, ckd.devtype))
    for t in range(ckd.ntrk):
        ckd.format_track(t, bpt, bs)
    edf = EDF(ckd)
    edf.bs, edf.bpt = bs, bpt
    total = ckd.ntrk * bpt
    edf.total, edf.dop = total, 4
    lab = bytearray(bs)
    lab[0:4] = ebc('CMS1')
    lab[4:10] = ebc(volid.upper(), 6)
    struct.pack_into('>8I', lab, 12, bs, 4, ckd.ncyl, ckd.ncyl, total, 0, 64, bs // 64)
    lab[44:50] = fst_date()
    edf.label = lab
    # allocation map: one bit a block, in whole blocks
    ambytes = (total + 7) // 8
    amblocks = (ambytes + bs - 1) // bs
    bits = bytearray(amblocks * bs)
    for n in range(total + 1, amblocks * bs * 8 + 1):
        edf.setbit(bits, n, True)          # past the end: never free
    for n in (1, 2, 3, 4):
        edf.setbit(bits, n, True)
    amb = edf.alloc(bits, amblocks)
    amfop, amlvl = build_tree(edf, bits, amb, 4)
    dirent = make_fst(b'\x00\x00\x00\x01\x00\x00\x00\x00', ebc('DIRECTOR'), 'P0',
                      'F', 64, 4, 1, 2, 0, 4)
    amfst = make_fst(b'\x00\x00\x00\x02\x00\x00\x00\x00', ebc('ALLOCMAP'), 'P0',
                     'F', bs, amfop, amblocks, amblocks, amlvl, 4)
    blk = (dirent + amfst).ljust(bs, b'\x00')
    edf.write(4, blk)
    for i, b in enumerate(amb):
        edf.write(b, bytes(bits[i * bs:(i + 1) * bs]))
    edf.used = sum(1 for n in range(1, total + 1) if edf.getbit(bits, n))
    edf.write_label()
    return edf


def main(argv):
    cckd = '--cckd' in argv
    argv = [a for a in argv if a != '--cckd']
    if len(argv) < 2:
        print(__doc__)
        return 2
    img, cmd = argv[0], argv[1].lower()
    tmp = None
    if cckd:
        tmp = tempfile.mkdtemp()
        raw = os.path.join(tmp, 'disk.ckd')
        subprocess.run(['cckd2ckd', img, raw], check=True,
                       stdout=subprocess.DEVNULL)
        path = raw
    else:
        path = img
    ckd = CKD(path)
    changed = False
    try:
        if cmd == 'format':
            edf = do_format(ckd, argv[2], int(argv[3]) if len(argv) > 3 else 4096)
            print('formatted %s: %d cylinders of %s, %d blocks of %d, %d used'
                  % (argv[2].upper(), ckd.ncyl, DEVNAME.get(ckd.devtype, '?'),
                     edf.total, edf.bs, edf.used))
            changed = True
            return 0
        edf = EDF(ckd).open()
        if cmd == 'label':
            print('volid %s blksize %d dop %d cyl %d/%d blocks %d used %d fst %d x %d'
                  % (edf.volid, edf.bs, edf.dop, edf.fcyl, edf.maxcyl, edf.total,
                     edf.used, edf.fstsize, edf.fstper))
        elif cmd == 'list':
            for i, f in edf.files():
                print('%-8s %-8s %-2s %s %6d %8d %6d L%d %s' % (
                    asc(f['fn']).strip(), asc(f['ft']).strip(), f['fmode'],
                    f['recfm'], f['lrecl'], f['aic'], f['adbc'], f['levels'],
                    f['date']))
        elif cmd == 'get':
            fn, ft = ebc(argv[2].upper(), 8), ebc(argv[3].upper(), 8)
            for i, f in edf.files():
                if f['fn'] == fn and f['ft'] == ft:
                    recs = edf.records_of(f)
                    out = open(argv[4], 'w') if len(argv) > 4 else sys.stdout
                    for r in recs:
                        out.write(asc(r).rstrip() + '\n')
                    return 0
            print('edf: file not found')
            return 28
        elif cmd in ('put', 'erase'):
            fn, ft = ebc(argv[2].upper(), 8), ebc(argv[3].upper(), 8)
            d = edf.directory()
            bits, am = edf.amap()
            entries = [f['raw'] for i, f in d]
            for i, f in d[2:]:
                if f['fn'] == fn and f['ft'] == ft:
                    free_file(edf, bits, f)
                    entries[i] = bytes(64)
            if cmd == 'put':
                fm = argv[4]
                lines = open(argv[5], encoding='latin-1').read().split('\n')
                if lines and lines[-1] == '':
                    lines.pop()
                recfm = argv[6].upper() if len(argv) > 6 else 'V'
                if recfm == 'F':
                    lrecl = int(argv[7]) if len(argv) > 7 else 80
                    recs = [ebc(l, lrecl) for l in lines]
                else:
                    recs = [ebc(l.rstrip() or ' ') for l in lines]
                    lrecl = max([len(r) for r in recs] or [1])
                fop, adbc, aic, levels, psz = write_file(edf, bits, recs, recfm, lrecl)
                e = make_fst(fn, ft, fm, recfm, lrecl, fop, adbc, aic, levels, psz)
                try:
                    k = entries.index(bytes(64), 2)
                    entries[k] = e
                except ValueError:
                    entries.append(e)
            while len(entries) > 2 and entries[-1] == bytes(64):
                entries.pop()
            commit(edf, entries, bits, am)
            changed = True
            print('%s %s: done, %d blocks used' % (argv[2].upper(), argv[3].upper(), edf.used))
        else:
            print(__doc__)
            return 2
        return 0
    finally:
        ckd.close()
        if cckd and changed:
            subprocess.run(['ckd2cckd', path, img + '.new'], check=True,
                           stdout=subprocess.DEVNULL)
            os.replace(img + '.new', img)
        if tmp:
            for x in os.listdir(tmp):
                os.unlink(os.path.join(tmp, x))
            os.rmdir(tmp)


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
