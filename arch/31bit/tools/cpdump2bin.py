#!/usr/bin/env python3
"""cpdump2bin.py LISTING OUT [N] -- the N-th (default 1) CP DUMP of a
virtual machine in a printer listing (Hercules 1403 file) as a binary
storage image, for adtdump.py (I-297).  Lines are "address word word ...
*chars*"; a line saying the following lines are the same as the one
before is honoured by repeating that line's 32 bytes."""
import re, sys
lines = open(sys.argv[1], 'rb').read().decode('latin-1').splitlines()
want = int(sys.argv[3]) if len(sys.argv) > 3 else 1
img = bytearray(); dumps = 0; prev_addr = None; last = None
hexrow = re.compile(r'^\s*([0-9A-F]{6,8})\s+((?:[0-9A-F]{8}\s+){1,8})')
for l in lines:
    m = hexrow.match(l)
    if not m:
        if 'SAME' in l.upper() and last is not None:
            # a run of identical lines up to the next address
            img_same = last
            continue
        continue
    addr = int(m.group(1), 16)
    if addr == 0 and (prev_addr is None or prev_addr != 0):
        dumps += 1
        if dumps > want: break
        img = bytearray()
    if dumps != want:
        prev_addr = addr; continue
    words = m.group(2).split()
    data = bytes.fromhex(''.join(words))
    if addr > len(img):
        # the gap: lines "SAME AS ABOVE" -- repeat the last line
        while len(img) < addr and last:
            img += last[:min(len(last), addr - len(img))]
        if len(img) < addr: img += bytes(addr - len(img))
    img[addr:addr + len(data)] = data
    last = data; prev_addr = addr
open(sys.argv[2], 'wb').write(img)
print('dump %d: %d bytes' % (want, len(img)))
