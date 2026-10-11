#!/usr/bin/env python3
"""mknative.py WORK OUT -- GnuCOBOL's runtime (libcob) and the GMP subset it
uses, prepared for GCC380/GCCLIB31 on CMS (9.19, native COBOL).

Each source is preprocessed here, on the PC, against GCCLIB31's headers and
the small POSIX layer in this directory (inc/, cobposix.h/.c), with the
macros GCC380 predefines and character constants evaluated in EBCDIC.  So
CMS sees self-contained C files: no include paths, no long file names.

GCC380 cuts external names to 8 characters and upper-cases them, so every
external name defined here is mapped to a unique CBnnnnnn by cobmap.h,
which is force-included into every file (and into LIBCOB H / GMP H for the
programs cobc generates).

WORK: gnucobol-3.1.2/ and gmp-6.3.0+dfsg/ (sources), gmpc/ (GMP configured
by mkcobol-cms.sh: config.h, gmp.h, tables), bc/ (libcob configured: its
config.h is the base of ours).
OUT:  src/*.c (CMS fn = file name), h/*.h (the headers for user programs),
      files.txt (CMS fn, origin), cobmap.h."""
import os, re, subprocess, sys, glob, shutil

H = os.path.dirname(os.path.abspath(__file__))
W, OUT = sys.argv[1], sys.argv[2]
G = os.path.join(W, 'gnucobol-3.1.2')
GS = os.path.join(W, 'gmp-6.3.0+dfsg')
GB = os.path.join(W, 'gmpc')
GCCLIB = os.path.normpath(os.path.join(H, '../../../gcclib31/src'))
TMP = os.path.join(OUT, 'tmp')
for d in (OUT, TMP, os.path.join(OUT, 'src'), os.path.join(OUT, 'h')):
    os.makedirs(d, exist_ok=True)

# ------------------------------------------------------------ configs
def edit(text, undef=(), define=()):
    for n in undef:
        text = re.sub(r'^#define %s\b.*$' % n, '/* #undef %s (CMS native) */' % n, text, flags=re.M)
    for n, v in define:
        text = re.sub(r'^#define %s\b.*$' % n, '', text, flags=re.M)
        text = re.sub(r'^/\* #undef %s \*/$' % n, '', text, flags=re.M)
        text += '\n#define %s %s\n' % (n, v)
    return text

cob = open(os.path.join(W, 'bc/config.h')).read()
cob = edit(cob,
    undef=['HAVE_FMEMOPEN', 'HAVE_DECL_FMEMOPEN', 'HAVE_LANGINFO_CODESET', 'HAVE_INTTYPES_H',
           'HAVE_STDINT_H', 'HAVE_WCHAR_H', 'HAVE_MALLOC_H', 'HAVE_MEMORY_H', 'HAVE_SYS_TIME_H',
           'HAVE_GETTIMEOFDAY', 'HAVE_SYS_WAIT_H', 'HAVE_DECL_FDATASYNC', 'HAVE_FDATASYNC',
           'HAVE_ISFINITE', 'HAVE_SIGACTION', 'HAVE_CLOCK_GETTIME', 'HAVE_FORK', 'HAVE_WAITPID',
           'HAVE_NANOSLEEP', 'HAVE_SYS_RESOURCE_H', 'HAVE_SYS_SELECT_H', 'HAVE_REALPATH',
           'HAVE_CANONICALIZE_FILE_NAME', 'HAVE_SYS_SOCKET_H', 'HAVE_POLL_H',
           'COB_LI_IS_LL', 'COB_64_BIT_POINTER', 'COB_COMPUTED_GOTO', 'HAVE_ATTRIBUTE_ALIGNED',
           'HAVE_DESIGNATED_INITS', 'HAVE_LOCALECONV', 'HAVE_STRCOLL', 'HAVE_RAISE'],
    define=[('COB_32_BIT_LONG', '1'), ('HAVE_8DOT3_FILENAMES', '1'),
            ('COB_CONFIG_DIR', '"."'), ('COB_COPY_DIR', '"."')])
cob += '\n#include "cobposix.h"\n'
open(os.path.join(TMP, 'config.h'), 'w').write(cob)

gmp = open(os.path.join(GB, 'config.h')).read()
gmp = edit(gmp,
    undef=['WANT_TMP_ALLOCA', 'HAVE_ALLOCA', 'HAVE_ALLOCA_H', 'HAVE_DOUBLE_IEEE_BIG_ENDIAN',
           'HAVE_LANGINFO_H', 'HAVE_NL_LANGINFO',
           'HAVE_SYS_PARAM_H', 'HAVE_SYS_RESOURCE_H', 'HAVE_SYS_TIME_H', 'HAVE_SYS_TIMES_H',
           'HAVE_HIDDEN_ALIAS', 'HAVE_LONG_DOUBLE', 'HAVE_STRNLEN', 'HAVE_INTMAX_T',
           'HAVE_GETRUSAGE', 'HAVE_GETTIMEOFDAY', 'HAVE_TIMES', 'HAVE_CLOCK_GETTIME',
           'HAVE_SYS_MMAN_H', 'HAVE_MMAP', 'HAVE_LOCALECONV', 'HAVE_RAISE', 'HAVE_ATTRIBUTE_MODE',
           'HAVE_STD__LOCALE', 'HAVE_SYS_SYSCTL_H', 'HAVE_SYS_SYSINFO_H', 'HAVE_SYS_STAT_H',
           'HAVE_SYS_TYPES_H', 'HAVE_UNISTD_H', 'HAVE_MEMORY_H', 'HAVE_STRINGS_H'],
    define=[('WANT_TMP_REENTRANT', '1')])
gmp += '\n#include "cobposix.h"\n'
gtmp = os.path.join(TMP, 'gmp')
os.makedirs(gtmp, exist_ok=True)
open(os.path.join(gtmp, 'config.h'), 'w').write(gmp)
for f in ('gmp.h', 'gmp-mparam.h', 'fac_table.h', 'fib_table.h', 'mp_bases.h',
          'trialdivtab.h', 'sieve_table.h', 'mpn/perfsqr.h'):
    shutil.copy(os.path.join(GB, f), gtmp)

# ------------------------------------------------------------ sources
srcs = []      # (cms fn, path, extra cpp args)
for f in sorted(glob.glob(os.path.join(G, 'libcob', '*.c'))):
    n = os.path.basename(f)[:-2]
    srcs.append(('CB' + n.upper()[:6], f, ['-I' + TMP, '-I' + G, '-I' + os.path.join(G, 'libcob'), '-I' + os.path.join(TMP, 'gmp')]))
srcs.append(('CBPOSIX', os.path.join(H, 'cobposix.c'), ['-I' + TMP]))
objs = [l.strip() for l in open(os.path.join(H, 'gmpobjs.txt')) if l.strip()]
for i, o in enumerate(objs):
    d, b = os.path.split(o)
    b = b[:-2]
    src = os.path.realpath(os.path.join(GB, d, b + '.c')) if d else None
    if not src or not os.path.exists(src):
        cands = glob.glob(os.path.join(GS, d or '.', b + '.c')) + glob.glob(os.path.join(GS, b + '.c'))
        src = cands[0]
    extra = ['-I' + gtmp, '-I' + GS, '-D__GMP_WITHIN_GMP']
    if d == 'mpn':
        extra.append('-DOPERATION_' + b)
    srcs.append(('GM%03d' % (i + 1), src, extra))

PRE = ['gcc', '-E', '-P', '-undef', '-nostdinc', '-std=gnu99', '-fexec-charset=IBM1047',
       '-funsigned-char', '-D__GNUC__=3', '-D__GNUC_MINOR__=2', '-D__GNUC_PATCHLEVEL__=3',
       '-D__CMS__', '-D__GCC31__', '-D__CHAR_UNSIGNED__', '-D__i370__', '-DNDEBUG',
       '-I' + os.path.join(H, 'inc'), '-I' + H, '-I' + GCCLIB]

def cpp(src, extra, out, forced=None):
    cmd = PRE + (['-include', forced] if forced else []) + extra + [src, '-o', out]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode:
        sys.exit('cpp failed: %s\n%s' % (src, r.stderr[-3000:]))

# phase 1: preprocess, compile for the symbols
X = 's390x-linux-gnu-gcc'
CC = [X, '-m31', '-std=gnu99', '-funsigned-char', '-w', '-c',
      '-Werror=implicit-function-declaration', '-fno-builtin']
ours, used = set(), set()
bad = []
for fn, src, extra in srcs:
    i = os.path.join(TMP, fn + '.i')
    cpp(src, extra, i)
    o = os.path.join(TMP, fn + '.o')
    r = subprocess.run(CC + ['-x', 'c', i, '-o', o], capture_output=True, text=True)
    if r.returncode:
        bad.append((fn, src, r.stderr))
        continue
    nm = subprocess.run(['s390x-linux-gnu-nm', o], capture_output=True, text=True).stdout
    for l in nm.split('\n'):
        p = l.split()
        if len(p) == 3 and p[1] in 'TDBRCGSV':
            ours.add(p[2])
        elif len(p) == 2 and p[0] == 'U':
            used.add(p[1])
if bad:
    for fn, src, err in bad:
        print('=== %s (%s)\n%s' % (fn, src, err[-2500:]))
    sys.exit('%d files failed the syntax check' % len(bad))

# the cobc-generated programs call libcob through common.h: declare every
# name too, so the map covers what LIBCOB H declares but nothing here defines
ext = sorted(ours)
libc = sorted(used - ours)
open(os.path.join(OUT, 'libc-used.txt'), 'w').write('\n'.join(libc) + '\n')

def b36(n):
    s = ''
    for _ in range(6):
        s = '0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ'[n % 36] + s
        n //= 36
    return s
m = ['/* cobmap.h -- generated by mknative.py: GCC380 keeps 8 characters of an',
     '   external name, so every external name of libcob, GMP and the POSIX',
     '   layer gets its own.  Included first everywhere. */',
     '#ifndef COBMAP_H', '#define COBMAP_H']
for k, n in enumerate(ext):
    if n == 'main':
        continue
    m.append('#define %s CB%s' % (n, b36(k)))
m.append('#endif')
cmap = os.path.join(OUT, 'cobmap.h')
open(cmap, 'w').write('\n'.join(m) + '\n')
print(len(ext), 'external names mapped;', len(libc), 'from GCCLIB31')

# phase 2: final sources, packed onto 80-column cards: C is free-form, so
# lines are joined (directives keep their own line); a line that is too long
# is split at a blank outside literals, else with backslash-newline
def split_long(l):
    out = []
    while len(l) > 80:
        cut, q = -1, None
        i = 0
        while i < 80:
            c = l[i]
            if q:
                if c == '\\':
                    i += 1
                elif c == q:
                    q = None
            elif c in '"\'':
                q = c
            elif c == ' ':
                cut = i
            i += 1
        if cut > 0:
            out.append(l[:cut]); l = l[cut + 1:]
        else:
            out.append(l[:79] + '\\'); l = l[79:]
    out.append(l)
    return out

def wrap(text, pack=True):
    out, cur = [], ''
    for l in text.split('\n'):
        l = l.replace('\t', ' ').strip() if pack else l.rstrip().replace('\t', ' ')
        if not l.strip():
            continue
        if not pack or l.startswith('#') or cur.endswith('\\'):
            if cur:
                out.extend(split_long(cur)); cur = ''
            if l.startswith('#') or not pack:
                out.extend(split_long(l))
                continue
        if cur and len(cur) + 1 + len(l) <= 80:
            cur += ' ' + l
        else:
            if cur:
                out.extend(split_long(cur))
            cur = l
    if cur:
        out.extend(split_long(cur))
    return '\n'.join(out) + '\n'

lst = []
for fn, src, extra in srcs:
    i = os.path.join(TMP, fn + '.i')
    cpp(src, extra, i, forced=cmap)
    open(os.path.join(OUT, 'src', fn + '.c'), 'w').write(wrap(open(i).read()))
    lst.append('%-8s %s' % (fn, os.path.relpath(src, W)))
# GCC380 predefines cms/CMS (and mvs): such an identifier in the C breaks it
bare = re.compile(r'\b(cms|CMS|mvs|MVS)\b')
for fn, src, extra in srcs:
    code = re.sub(r'"(\\.|[^"\\])*"|\'(\\.|[^\'\\])*\'', '""', open(os.path.join(OUT, 'src', fn + '.c')).read())
    if bare.search(code):
        sys.exit('%s: an identifier GCC380 predefines: %s' % (fn, bare.search(code).group(0)))
open(os.path.join(OUT, 'files.txt'), 'w').write('\n'.join(lst) + '\n')

# headers for the programs cobc writes: LIBCOB H, COBCOMM H, COBEXC H,
# COBEXCIO H, GMP H, COBMAP H
def hdr(src, name, repl=()):
    t = open(src, encoding='latin-1').read()
    for a, b in repl:
        t = t.replace(a, b)
    open(os.path.join(OUT, 'h', name), 'w').write(wrap('#include "cobmap.h"\n' + t, pack=False))
hdr(os.path.join(G, 'libcob.h'), 'libcob.h', [('<libcob/common.h>', '"cobcomm.h"')])
hdr(os.path.join(G, 'libcob/common.h'), 'cobcomm.h', [('<libcob/exception.def>', '"cobexc.h"')])
hdr(os.path.join(G, 'libcob/exception.def'), 'cobexc.h', [('"exception-io.def"', '"cobexcio.h"')])
hdr(os.path.join(G, 'libcob/exception-io.def'), 'cobexcio.h')
hdr(os.path.join(gtmp, 'gmp.h'), 'gmp.h')
shutil.copy(cmap, os.path.join(OUT, 'h', 'cobmap.h'))
print(len(srcs), 'sources in', os.path.join(OUT, 'src'))
