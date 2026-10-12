#!/usr/bin/env python3
"""mkcmspli.py PLISRC PLI2C.TEXT IO -- the reader decks for PL/I on CMS:
IO/pli2c.rdr (PLI2C TEXT: the translator, cross-built, M5f) and
IO/plifiles.txt (READCARD *: PLIC EXEC, PLITEST EXEC, PLIRT H with the
CMS prelude PLICMS H, the tests as Tnn PLI with their Tnn IN input)."""
import glob, os, sys
src, text, io = sys.argv[1:4]
H = os.path.dirname(os.path.abspath(__file__))
card = 'ID CMSUSER NAME PLI2C TEXT'.ljust(80).encode('cp037')
open(os.path.join(io, 'pli2c.rdr'), 'wb').write(card + open(text, 'rb').read())
L = ['ID CMSUSER NAME PLI FILES']
def add(fn, ft, lines):
    for l in lines:
        assert len(l) <= 80, (fn, l)
    L.append(':READ  %-8s %-8s A1' % (fn, ft))
    L.extend(lines)
def rd(p):
    return [l.rstrip() for l in open(p).read().rstrip('\n').split('\n')]
add('PLIC', 'EXEC', rd(os.path.join(H, 'cms/PLIC.EXEC')))
add('PLICMS', 'H', rd(os.path.join(H, 'cms/PLICMS.H')))
add('PLIRT', 'H', ['#include "plicms.h"'] + rd(os.path.join(src, 'src/plirt.h')))
for f in sorted(glob.glob(os.path.join(src, 'tests/*.inc'))):
    b = os.path.basename(f)[:-4]
    add(b.upper()[:8], 'INC', rd(f))
names = []
for f in sorted(glob.glob(os.path.join(src, 'tests/*.pli'))):
    n = os.path.basename(f)[:3].upper()
    add(n, 'PLI', rd(f))
    inp = f[:-4] + '.in'
    if not os.path.exists(inp):
        inp = f[:3] + f[3:].replace(os.path.basename(f), os.path.basename(f)[:4] + 'data.in')
    if os.path.exists(inp):
        add(n, 'IN', rd(inp))
    names.append(n)
t = ['/* PLITEST EXEC -- PLIC and run every test (stdin from fn IN) */',
     'ARG LIST', 'DO I = 1 TO WORDS(LIST)', '  F = WORD(LIST, I)',
     "  'PLIC' F", "  IF RC <> 0 THEN DO; SAY 'PLITEST:' F 'BUILD RC' RC; ITERATE; END",
     "  'MAKEBUF'", "  'STATE' F 'IN A'", "  IF RC = 0 THEN DO",
     "    'EXECIO * DISKR' F 'IN A (FINIS STEM IN.'",
     "    DO J = 1 TO IN.0; QUEUE IN.J; END", '  END',
     "  SAY '=== PLITEST' F", "  F", "  R = RC", "  'DROPBUF'", "  SAY '=== END' F 'RC' R", 'END', 'EXIT 0']
add('PLITEST', 'EXEC', t)
open(os.path.join(io, 'plifiles.txt'), 'w').write('\n'.join(L) + '\n')
print(' '.join(names))
