#!/usr/bin/env python3
"""mkcmspy.py PYSRC OUT -- Python (mikebeer/python) for CMS, 9.15:
OUT/py.ximage (PY XIMAGE: py_embed_all.c by m5f/cmscc for the XL loader --
2.4 MB, more than a TEXT deck may hold),
OUT/files/ and OUT/py.lst for tools/edf.py putlist: PYTHON EXEC, PYTEST EXEC,
the tests as PYTnn PY (names cut to 8), and OUT/map.txt (PYTnn -> test)."""
import glob, os, subprocess, sys
src, out = sys.argv[1:3]
H = os.path.dirname(os.path.abspath(__file__))
M = os.path.join(H, '../../m5f')
os.makedirs(os.path.join(out, 'files'), exist_ok=True)
c = os.path.join(src, 'c')
# 32-bit long: float -> int beyond SI_MAX must become a big integer
# (python-32bit-long.patch, until mikebeer/python has it)
pat = os.path.join(H, 'python-32bit-long.patch')
if subprocess.run(['git', '-C', src, 'apply', '--check', pat], capture_output=True).returncode == 0:
    subprocess.run(['git', '-C', src, 'apply', pat], check=True)
subprocess.run(['make', '-C', c, 'embed'], check=True, stdout=subprocess.DEVNULL)
elf = os.path.join(out, 'py.elf')
subprocess.run([os.path.join(M, 'cmscc'), '-O2', '-DPY_EMBED_LIBS', '-o', elf,
                os.path.join(c, 'py_embed_all.c')], check=True, env=dict(os.environ, CMSCC_XL='1'))
subprocess.run(['python3', os.path.join(M, 'mkximage.py'), elf, os.path.join(out, 'py.ximage')], check=True)
L = ['PY XIMAGE A1 %s BIN' % os.path.abspath(os.path.join(out, 'py.ximage'))]
for f in ('PYTHON.EXEC', 'PYTEST.EXEC'):
    L.append('%s %s A1 %s F 80' % (f.split('.')[0], f.split('.')[1], os.path.join(H, 'cms', f)))
mp = []
for f in sorted(glob.glob(os.path.join(src, 'tests', '[0-9][0-9]_*.py'))):
    n = 'PYT' + os.path.basename(f)[:2]
    L.append('%s PY A1 %s V' % (n, os.path.abspath(f)))
    mp.append('%s %s' % (n, os.path.basename(f)[:-3]))
open(os.path.join(out, 'py.lst'), 'w').write('\n'.join(L) + '\n')
open(os.path.join(out, 'map.txt'), 'w').write('\n'.join(mp) + '\n')
print(' '.join(m.split()[0] for m in mp))
