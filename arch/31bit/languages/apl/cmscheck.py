#!/usr/bin/env python3
"""cmscheck.py LOG TESTS.json -- compare APL output captured from the console
log of a CMS run (apl FN.apl ... Ready;) with the reference outputs.
Lines that are exactly two blanks are the 3215's line turnaround, not output."""
import json, re, sys
log, tj = sys.argv[1:3]
tests = json.load(open(tj))
out = {}; cur = None
for l in open(log, errors='replace').read().split('\n'):
    m = re.match(r'^\d\d:\d\d:\d\d apl (\S+)\.apl$', l)
    if m: cur = m.group(1); out[cur] = []; continue
    if cur is None or 'HHC' in l: continue
    m = re.match(r'^\d\d:\d\d:\d\d /(.*)$', l)
    if not m: continue
    t = m.group(1)
    if t.startswith('Ready'): cur = None; continue
    if t == '  ': continue
    out[cur].append(t.rstrip())
ok = 0
for n, fn, exp in tests:
    got = out.get(fn, [])
    while got and got[-1] == '': got.pop()
    e = [x.rstrip() for x in open(exp).read().rstrip('\n').split('\n')]
    if got == e: ok += 1
    else: print('FAIL', n)
print('passed %d of %d' % (ok, len(tests)))
