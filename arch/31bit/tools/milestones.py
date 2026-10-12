#!/usr/bin/env python3
"""milestones.py [ID STATUS [NOTES]] -- the milestone plan.

docs/VM370PLUS-Milestones.xlsx is the source (header row 3, rows from 4);
docs/60-MILESTONES.md is generated from it: its text before the table is
kept, the table is rewritten.  With ID and STATUS (and NOTES), that row is
updated first.  The xlsx is copied to dist/."""
import os, shutil, sys
import openpyxl
R = os.path.abspath(os.path.join(os.path.dirname(__file__), '../../..'))
X = os.path.join(R, 'docs/VM370PLUS-Milestones.xlsx')
M = os.path.join(R, 'docs/60-MILESTONES.md')
wb = openpyxl.load_workbook(X)
ws = wb.active
if len(sys.argv) > 2:
    rid = sys.argv[1]
    for r in ws.iter_rows(min_row=4):
        if str(r[1].value).strip() == rid:
            r[4].value = sys.argv[2]
            if len(sys.argv) > 3:
                r[6].value = sys.argv[3]
            break
    else:
        sys.exit('milestones: no row ' + rid)
    wb.save(X)
rows = []
for r in ws.iter_rows(min_row=4):
    v = [('' if c.value is None else str(c.value)) for c in r][:7]
    if not any(x.strip() for x in v):
        continue
    rows.append('| ' + ' | '.join(x.replace('\n', ' ').replace('|', '\\|') for x in v) + ' |')
md = open(M).read().split('\n')
first = next(k for k, l in enumerate(md) if l.startswith('| '))
last = max(k for k, l in enumerate(md) if l.startswith('| ') or l.startswith('|---'))
out = md[:first] + rows[:1] + ['|---|---|---|---|---|---|---|'] + rows[1:] + md[last + 1:]
open(M, 'w').write('\n'.join(out))
os.makedirs(os.path.join(R, 'dist'), exist_ok=True)
shutil.copy(X, os.path.join(R, 'dist'))
print('milestones: %d rows' % len(rows))
