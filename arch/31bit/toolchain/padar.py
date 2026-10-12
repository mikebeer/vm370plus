#!/usr/bin/env python3
"""padar.py lib.a ... -- make each archive's length a multiple of 80 (cards
pad the last record; a trailing partial member would break the archive)
by adding a member zzpad of newlines that no symbol index entry names."""
import os, subprocess, sys, tempfile
for a in sys.argv[1:]:
    n = os.path.getsize(a)
    if n % 80 == 0:
        continue
    for k in range(1, 400):
        if (n + 60 + k + (k & 1)) % 80 == 0:
            break
    d = tempfile.mkdtemp()
    p = os.path.join(d, 'zzpad')
    open(p, 'wb').write(b'\n' * k)
    subprocess.run(['s390x-linux-gnu-ar', 'q', a, p], check=True)
    print(a, n, '->', os.path.getsize(a))
