#!/usr/bin/env python3
"""mkdeck.py NAT DECK [USER] -- the reader deck (ASCII cards, devinit ...
ascii eof trunc; READCARD *) that brings the mknative.py output to CMS:
COBNAT EXEC/PARM/FILES and every source as fn C on C1, the headers for
user programs (LIBCOB H ...) as fn H on C1."""
import os, sys
H = os.path.dirname(os.path.abspath(__file__))
nat, deck = sys.argv[1], sys.argv[2]
user = sys.argv[3] if len(sys.argv) > 3 else 'MAINT'
L = ['ID %s NAME COBNAT DECK' % user]
def add(fn, ft, lines, fm='C1'):
    for l in lines:
        assert len(l) <= 80, (fn, l)
    L.append(':READ  %-8s %-8s %s' % (fn, ft, fm))
    L.extend(lines)
def lines(p):
    return open(p).read().rstrip('\n').split('\n')
add('COBNAT', 'EXEC', [l.rstrip() for l in lines(os.path.join(H, 'cms/COBNAT.EXEC'))])
add('COBNAT', 'PARM', lines(os.path.join(H, 'cms/COBNAT.PARM')))
add('COBNAT0', 'PARM', [l.replace('-O1', '-O0') for l in lines(os.path.join(H, 'cms/COBNAT.PARM'))])
fl = lines(os.path.join(nat, 'files.txt'))
only = os.environ.get('ONLY', '').split()
if only:
    fl = [l for l in fl if l.split()[0] in only]
add('COBNAT', 'FILES', [l[:80] for l in fl])
for l in fl:
    fn = l.split()[0]
    add(fn, 'C', lines(os.path.join(nat, 'src', fn + '.c')))
for h in sorted(os.listdir(os.path.join(nat, 'h'))):
    add(h[:-2].upper(), 'H', lines(os.path.join(nat, 'h', h)))
open(deck, 'w').write('\n'.join(L) + '\n')
print(len(L), 'cards')
