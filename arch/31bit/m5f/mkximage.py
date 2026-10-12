#!/usr/bin/env python3
"""mkximage.py PROG.elf PROG.ximage -- a large M5f program (linked with
imagexl.ld at address 0, --emit-relocs) as an XIMAGE for the XL loader:

  header  'XIMG', version 1, entry, image bytes, bss bytes, relocations
          (big-endian 32-bit words)
  image   the loadable bytes from address 0
  relocs  one 32-bit offset per R_390_32 address constant: the loader adds
          the address it loaded the image at

PC-relative references are already resolved by the static link (as for
elf_to_cms), so R_390_32 against a section symbol or a defined symbol is
the only kind that moves."""
import struct, sys

src, dst = sys.argv[1], sys.argv[2]
d = open(src, 'rb').read()
assert d[:4] == b'\x7fELF' and d[4] == 1 and d[5] == 2, 'not ELF32 big-endian'
(e_type, e_machine, e_version, e_entry, e_phoff, e_shoff, e_flags, e_ehsize,
 e_phentsize, e_phnum, e_shentsize, e_shnum, e_shstrndx) = struct.unpack('>HHIIIIIHHHHHH', d[16:52])
assert e_machine == 22, 's390 expected'
shs = [struct.unpack('>IIIIIIIIII', d[e_shoff + i * e_shentsize:e_shoff + (i + 1) * e_shentsize]) for i in range(e_shnum)]
phs = [struct.unpack('>IIIIIIII', d[e_phoff + i * e_phentsize:e_phoff + (i + 1) * e_phentsize]) for i in range(e_phnum)]
loads = [p for p in phs if p[0] == 1]
end_file = max(p[2] + p[4] for p in loads)       # vaddr + filesz
end_mem = max(p[2] + p[5] for p in loads)        # vaddr + memsz
assert min(p[2] for p in loads) == 0, 'link at address 0 (imagexl.ld)'
img = bytearray(end_file)
for p in loads:
    img[p[2]:p[2] + p[4]] = d[p[1]:p[1] + p[4]]
SHF_ALLOC = 2
symtab = [s for s in shs if s[1] == 2]
syms = []
if symtab:
    s = symtab[0]
    for i in range(s[5] // 16):
        syms.append(struct.unpack('>IIIBBH', d[s[4] + i * 16:s[4] + i * 16 + 16]))
rel = []
for s in shs:
    if s[1] != 4:                                # SHT_RELA
        continue
    tgt = shs[s[7]]                              # sh_info: the section relocated
    if not (tgt[2] & SHF_ALLOC):
        continue
    for i in range(s[5] // 12):
        off, info, add = struct.unpack('>IIi', d[s[4] + i * 12:s[4] + i * 12 + 12])
        if info & 0xff != 4:                     # R_390_32
            continue
        sym = syms[info >> 8] if syms else None
        if sym and sym[5] == 0xfff1:             # SHN_ABS: a number, not an address
            continue
        if off + 4 > end_file:
            continue                             # in BSS: nothing stored
        rel.append(off)
# a static link fills .got with absolute addresses and emits no relocation
# for them (libgcc's -fPIC objects reach __clz_tab that way): each nonzero
# word that is an address in the image moves with it (as elf_to_cms does)
shstr = shs[e_shstrndx]
for s in shs:
    nm = d[shstr[4] + s[0]:d.index(b'\0', shstr[4] + s[0])].decode()
    if nm == '.got' and s[1] == 1:
        for j in range(0, s[5] - 3, 4):
            w = struct.unpack('>I', img[s[3] + j:s[3] + j + 4])[0]
            if w and w <= end_mem:
                rel.append(s[3] + j)
rel = sorted(set(rel))
hdr = struct.pack('>4sIIIII', b'XIMG', 1, e_entry, end_file, end_mem - end_file, len(rel))
open(dst, 'wb').write(hdr + bytes(img) + b''.join(struct.pack('>I', r) for r in rel))
print('XIMAGE: image %d, bss %d, %d relocations, entry %#x' % (end_file, end_mem - end_file, len(rel), e_entry))
