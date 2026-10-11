#!/usr/bin/env python3
"""mkcmsapl.py APLTEXT IO TESTS.json -- the reader decks for APL on CMS:
IO/apl.rdr (ID CMSUSER card + the APL TEXT deck, EBCDIC) and IO/aplfiles.txt
(READCARD *: APL EXEC and the example/scenario scripts with a reference
output; scripts with lines over 80 columns cannot travel on cards and are
left out).  TESTS.json lists (name, CMS fn, reference) for cmscheck.py.
Run in the mikebeer/apl checkout.  tests/runs/apl-cms.json is the CMS run."""
import glob, json, os, sys
text, io, tj = sys.argv[1:4]
HERE = os.path.dirname(os.path.abspath(__file__))
card = 'ID CMSUSER NAME APL TEXT'.ljust(80).encode('cp037')
open(os.path.join(io, 'apl.rdr'), 'wb').write(card + open(text, 'rb').read())
L = ['ID CMSUSER NAME APL FILES']
def add(fn, ft, lines):
    L.append(':READ  %-8s %-8s A1' % (fn.upper()[:8], ft.upper()))
    L.extend(lines)
add('APL', 'EXEC', [l.rstrip() for l in open(os.path.join(HERE, 'cms/APL.EXEC')).read().rstrip('\n').split('\n')])
tests = []
for f in sorted(glob.glob('examples/*.apl')) + sorted(glob.glob('tests/scen/s*.apl')):
    n = os.path.basename(f)[:-4]
    exp = 'tests/scen/%s.out' % n if f.startswith('tests') else \
          ('ref/%s.c.out' % n if os.path.exists('ref/%s.c.out' % n) else 'ref/%s.out' % n)
    if not os.path.exists(exp): continue
    lines = open(f, encoding='latin-1').read().rstrip('\n').split('\n')
    if max(len(l) for l in lines) > 80:
        print('left out (over 80 columns):', n); continue
    add(n.upper()[:8], 'APL', lines); tests.append((n, n.upper()[:8], exp))
open(os.path.join(io, 'aplfiles.txt'), 'w').write('\n'.join(L) + '\n')
json.dump(tests, open(tj, 'w'))
print(len(tests), 'tests')
