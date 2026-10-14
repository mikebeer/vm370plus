#!/usr/bin/env python3
# gensigs.py ZIP... -- list every native method signature in the class
# archives, in gen-stubs.c's input format ("static (..)R" or "(..)R"), so
# JamVM on CMS has a C stub for each (no libffi there).  VM/370plus 9.35.
import sys, zipfile, importlib.util
spec = importlib.util.spec_from_file_location('javah', '/home/claude/src/java/fakebin/javah')
src = open('/home/claude/src/java/fakebin/javah').read().replace('\nmain()\n', '\n')
ns = {}; exec(compile(src, 'javah', 'exec'), ns)
sigs = set()
for z in sys.argv[1:]:
    with zipfile.ZipFile(z) as zf:
        for n in zf.namelist():
            if not n.endswith('.class'): continue
            _, methods = ns['parse'](zf.read(n))
            for acc, nm, ds, _ in methods:
                if acc & 0x100:
                    sigs.add(('static ' if acc & 8 else '') + ds)
for s in sorted(sigs): print(s)
