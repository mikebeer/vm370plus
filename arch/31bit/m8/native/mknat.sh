#!/bin/bash
# mknat.sh -- M8.2: current cREXX built natively on CMS with GCC 13 (9.21).
#
#   mknat.sh SYSROOT.tar OUTDIR
#
# Writes to OUTDIR:
#   crxroot.tar   CRXROOT TAR: LANG's SYSROOT TAR plus the cREXX sources and
#                 every header they reach, under /crexx (s = the source tree,
#                 h = the host build's generated C, g = generated headers,
#                 cms = m8/inc/cms).  CRX82 EXEC copies it to the build disk
#                 as SYSROOT TAR, so cc1 finds both there.
#   crx82.exec    CRX82 EXEC: cc1 + as per unit (restartable: a unit whose
#                 O file exists is skipped), ar, ld, elf2cms, LOAD, GENMOD
#                 -> RXBVM82, RXAS82, RXC82 MODULE.
#   pc/           the same recipe run on the PC (s390x gcc, the sysroot's
#                 headers and libraries): the reference TEXT decks.
# The sources are M8.1's (units.sh); the flags are CC EXEC's plus M8.1's.
set -e
H8=$(cd "$(dirname "$0")/.." && pwd)
SYSTAR=$(readlink -f "$1"); O=$(readlink -f "$2")
cd "$H8"; . ./units.sh
X=s390x-linux-gnu
E2C=${E2C:-/home/claude/m5f/elf_to_cms8}
mkdir -p "$O"; W=$O/root; rm -rf "$W" "$O/pc"; mkdir -p "$W/crexx" "$O/pc"
tar xf "$SYSTAR" -C "$W"
python3 - "$O" "$W" "$C" "$H" "$G" "$H8/inc/cms" "$CFLAGS_EXTRA" \
  "$C/interpreter/rxvmmain.c $C/assembler/rxasmain.c $C/compiler/rxc_main.c $C/interpreter/rxvml.c" \
  "$VMF" "$AS" "$CP" <<'EOF'
import os, sys, subprocess, shutil, tarfile, re
O, W, C, H, G, CMSI, CFX, MAINS, VMF, AS, CP = sys.argv[1:]
roots = [(C, '/crexx/s'), (H, '/crexx/h'), (G, '/crexx/g'), (CMSI, '/crexx/cms')]
def cms(p):                       # PC path -> path inside CRXROOT TAR
    p = os.path.realpath(p)
    for r, t in roots:
        r = os.path.realpath(r)
        if p == r or p.startswith(r + '/'): return t + p[len(r):]
    if p.startswith(W + '/'): return p[len(W):]
    raise SystemExit('mknat: outside the tree: ' + p)
# flags: M8.1's (-std=gnu99 -Os, DEFS, INC, -include cmstz.h) with the
# include directories moved into the tar; RXC's and RXAS's extra -I for all
words = CFX.split() + ['-I' + H + '/compiler', '-I' + H + '/assembler']
fl = []
for w in words:
    if w.startswith('-I'): fl.append('-I' + cms(w[2:]))
    else: fl.append(w)
base = ('-quiet -m31 -mesa -march=z900 -msoft-float -mlong-double-64 '
        '-fno-pic -fno-asynchronous-unwind-tables -fno-unwind-tables '
        '-fno-stack-protector -ffunction-sections -fdata-sections -nostdinc '
        '-I /usr/include/m5f -isystem /usr/lib/gcc/include '
        # cc1's garbage collector earlier: rxvmintp.c (the VM's run() with
        # its handlers inlined) ran the heap above 16 MB out in a 256M machine
        '--param=ggc-min-expand=20 --param=ggc-min-heapsize=32768 '
        '-D_Thread_local= -D__thread= -D__SINGLE_THREAD__ '
        '-isystem /usr/include/newlib').split()
# not CC EXEC's -D__CMS__ (cREXX's platform.h would take its GCCLIB route,
# cms.h) nor its -U__linux__ ...: M8.1's predefines, which cREXX's CMS ELF
# port was made with
# the VM's handlers out of line on CMS (route 1 did the same): with them
# inlined into run() (M8.1's panel 20) cc1 needs more heap than a 256M
# machine has
fl += ['-UCREXX_VM_HANDLER_PANEL', '-DCREXX_VM_HANDLER_PANEL=1']
allf = base + fl
# units and their 8-character CMS names
mains = MAINS.split(); vm = VMF.split(); asm = AS.split(); cp = CP.split()
units = []
for u in mains + vm + asm + cp:
    if u not in units: units.append(u)
names, used = {}, set()
for u in units:
    b = re.sub('[^A-Z0-9]', '', os.path.basename(u)[:-2].upper())
    n = b[:8]; k = 1
    while n in used:
        n = b[:8 - len(str(k))] + str(k); k += 1
    used.add(n); names[u] = n
# 1. the PC: dependencies, and the same objects (the reference)
pcf = [f.replace('/usr/', W + '/usr/', 1) if f.startswith('/usr/') else f for f in base if f != '-quiet']
fix = []
for f in fl:
    if f.startswith('-I/crexx/'):
        t = f[2:]; r = [r for r, tt in roots if t == tt or t.startswith(tt + '/')]
        tt = [tt for r2, tt in roots if t == tt or t.startswith(tt + '/')][0]
        fix.append('-I' + os.path.realpath(r[0]) + t[len(tt):])
    else: fix.append(f)
pcflags = []
it = iter(pcf)
for f in it:
    if f in ('-I', '-isystem'):
        d = next(it); pcflags += [f, W + d if d.startswith('/usr/') else d]
    else: pcflags.append(f)
files = set()
raw = set()     # include paths with .. or . as cc1 asks for them: cmsrt's
                # VFS matches names literally, so they are tar members too
os.makedirs(O + '/pc/o', exist_ok=True)
for u in units:
    o = O + '/pc/o/' + names[u] + '.o'
    subprocess.run(['s390x-linux-gnu-gcc'] + pcflags + fix + ['-MD', '-MF', o + '.d', '-c', u, '-o', o], check=True)
    d = open(o + '.d').read().replace('\\\n', ' ').split(':', 1)[1].split()
    files.update(os.path.realpath(x) for x in d)
    raw.update(x for x in d if '/../' in x or '/./' in x)
# 2. CRXROOT TAR
for f in sorted(files):
    if f.startswith(W + '/'): continue
    t = W + cms(f); os.makedirs(os.path.dirname(t), exist_ok=True); shutil.copy(f, t)
subprocess.run(['tar', '--format=ustar', '-cf', O + '/crxroot.tar', '-C', W, 'usr', 'crexx'], check=True)
def rawcms(x):
    for r, t in roots:
        for rr in (r, os.path.realpath(r)):
            if x.startswith(rr + '/'): return t + x[len(rr):]
    raise SystemExit('mknat: raw path outside the tree: ' + x)
import tarfile
with tarfile.open(O + '/crxroot.tar', 'a', format=tarfile.USTAR_FORMAT) as tf:
    for x in sorted(raw):
        tf.add(os.path.realpath(x), arcname=rawcms(x).lstrip('/'))
print('mknat: %d literal ../ paths' % len(raw))
open(O + '/units.txt', 'w').write(''.join('%-8s %s\n' % (names[u], cms(u)) for u in units))
# 3. CRX82 EXEC
def q(s):            # one QUEUE line of a long command (' \' continues)
    return "QUEUE '" + s.replace("'", "''") + " \\'"
def chunks(ws, n=60):
    line, out = '', []
    for w in ws:
        if len(line) + len(w) + 1 > n: out.append(line); line = ''
        line = (line + ' ' + w).strip()
    if line: out.append(line)
    return out
E = []
E.append("/* CRX82 EXEC -- M8.2: current cREXX built natively on CMS (GCC 13)  */")
E.append("/*   CRX82 [ALL|VM|AS|C|DONE] -- IPL 290, 256M; A = LANG R/W (I-297)*/")
E.append("/*   (cc1, as, ld, CRXROOT TAR and the objects on one EDF disk)    */")
E.append("/* written by m8/native/mknat.sh: do not edit                        */")
E.append("PARSE UPPER ARG WHAT .")
E.append("IF WHAT = '' THEN WHAT = 'ALL'")
E.append("/* cc1 reads SYSROOT TAR: CRXROOT (the sysroot plus /crexx) takes   */")
E.append("/* its name for the build; DONE gives the names back                */")
E.append("IF WHAT = 'DONE' THEN DO")
E.append("  'RENAME SYSROOT TAR A CRXROOT TAR A'")
E.append("  'RENAME SYSROOT0 TAR A SYSROOT TAR A'")
E.append("  EXIT RC")
E.append("END")
E.append("'STATE CRXROOT TAR A'")
E.append("IF RC = 0 THEN DO")
E.append("  'RENAME SYSROOT TAR A SYSROOT0 TAR A'")
E.append("  'RENAME CRXROOT TAR A SYSROOT TAR A'")
E.append("END")
E.append("'STATE SYSROOT0 TAR A'")
E.append("IF RC <> 0 THEN DO; SAY 'CRX82: CRXROOT TAR not on A'; EXIT 28; END")
E.append("CALL TIME 'R'")
for u in units:
    t = cms(u); E.append("P = '%s'" % t[:60])
    for k in range(60, len(t), 60): E.append("P = P || '%s'" % t[k:k + 60])
    E.append("CALL UNIT '%s', P" % names[u])
E.append("SAY 'CRX82: all units compiled' TIME('E') 's'")
lib = [names[u] for u in vm + asm if u not in mains]
seen = []
for n in lib:
    if n not in seen: seen.append(n)
E.append("IF WHAT = 'ALL' | WHAT = 'C' THEN DO")
E.append("  CALL ERASE 'CRXLIB A A'")
for c in chunks([n.lower() + '.o' for n in seen], 50):
    E.append("  " + q(c))
E.append("  QUEUE ''")
E.append("  'AR rcs crxlib.a =STACK'")
E.append("  IF RC <> 0 THEN EXIT RC")
E.append("END")
def link(mod, objs, lib):
    E.append("IF WHAT = 'ALL' | WHAT = '%s' THEN CALL LINK '%s', '%s', '%s'" % (
        {'RXBVM82': 'VM', 'RXAS82': 'AS', 'RXC82': 'C'}[mod], mod,
        ' '.join(n.lower() + '.o' for n in objs), lib))
    l = E.pop(); i = l.index(", '", l.index("CALL LINK") + 12)   # split: objs
    head, rest = l[:i + 1], l[i + 2:]
    E.append(head)
    o = rest[1:rest.index("'", 1)]; tail = rest[rest.index("'", 1) + 1:]
    parts = chunks(o.split(), 60)
    for k, c in enumerate(parts):
        E.append("  '" + c + (" ' ||," if k < len(parts) - 1 else "'" + tail))
link('RXBVM82', [names[mains[0]]] + [names[u] for u in vm], '')
link('RXAS82', [names[mains[1]]] + [names[u] for u in asm] + [names[u] for u in vm], '')
link('RXC82', [names[mains[2]]] + [names[u] for u in cp] + [names[mains[3]]], 'crxlib.a')
E.append("SAY 'CRX82: done' TIME('E') 's'")
E.append("EXIT 0")
E.append("")
E.append("UNIT: PROCEDURE   /* compile one unit: cc1 + as, unless its O exists */")
E.append("  PARSE ARG FN, SRC")
E.append("  'SET CMSTYPE HT'; 'STATE' FN 'O A'; S = RC; 'SET CMSTYPE RT'")
E.append("  IF S = 0 THEN RETURN")
E.append("  SAY 'CRX82:' FN SRC TIME('E') 's'")
for c in chunks(allf, 60):
    E.append("  " + q(c))
E.append("  QUEUE SRC '-o' FN'.s'")
E.append("  'XL CC1 =STACK'")
E.append("  IF RC <> 0 THEN DO; SAY 'CRX82: cc1 RC' RC 'for' SRC; EXIT RC; END")
E.append("  QUEUE '-m31 -mesa' FN'.s -o' FN'.o'")
E.append("  'AS =STACK'")
E.append("  IF RC <> 0 THEN DO; SAY 'CRX82: as RC' RC 'for' FN; EXIT RC; END")
E.append("  CALL ERASE FN 'S A'")
E.append("  RETURN")
E.append("")
E.append("LINK: PROCEDURE   /* ld -> ELF, elf2cms (8 MB) -> TEXT, LOAD, GENMOD */")
E.append("  PARSE ARG MOD, OBJS, LIB")
E.append("  SAY 'CRX82: linking' MOD TIME('E') 's'")
E.append("  QUEUE '-m elf_s390 -static --emit-relocs --gc-sections \\'")
E.append("  QUEUE '-T /usr/lib/image.ld -o' MOD'.elf /usr/lib/entry31.o \\'")
E.append("  DO WHILE OBJS <> ''")
E.append("    PARSE VAR OBJS O1 O2 O3 O4 O5 O6 OBJS")
E.append("    QUEUE O1 O2 O3 O4 O5 O6 '\\'")
E.append("  END")
E.append("  QUEUE '/usr/lib/cmsrt.o --start-group' LIB '/usr/lib/libc.a \\'")
E.append("  QUEUE '/usr/lib/libm.a /usr/lib/libsoftf.a /usr/lib/libgcc.a --end-group'")
E.append("  'LD =STACK'")
E.append("  IF RC <> 0 THEN DO; SAY 'CRX82: ld RC' RC 'for' MOD; EXIT RC; END")
E.append("  CALL ERASE MOD 'TEXT A'")
E.append("  QUEUE MOD'.elf' MOD'.text'")
E.append("  'XL ELF2CM8 =STACK'")
E.append("  IF RC <> 0 THEN DO; SAY 'CRX82: elf2cms RC' RC 'for' MOD; EXIT RC; END")
E.append("  CALL ERASE MOD 'ELF A'")
E.append("  'LOAD' MOD '(CLEAR NOMAP'")
E.append("  IF RC <> 0 THEN EXIT RC")
E.append("  'GENMOD' MOD")
E.append("  IF RC <> 0 THEN EXIT RC")
E.append("  SAY 'CRX82:' MOD 'MODULE built' TIME('E') 's'")
E.append("  RETURN")
E.append("")
E.append("ERASE: PROCEDURE")
E.append("  PARSE ARG ID")
E.append("  'SET CMSTYPE HT'; 'ERASE' ID; 'SET CMSTYPE RT'")
E.append("  RETURN")
for l in E:
    if len(l) > 80: raise SystemExit('mknat: EXEC line over 80: ' + l)
open(O + '/crx82.exec', 'w').write('\n'.join(E) + '\n')
# the PC link lists, for step 4 below
open(O + '/pc/links', 'w').write(
    'rxbvm82 %s\n' % ' '.join([names[mains[0]]] + [names[u] for u in vm]) +
    'rxas82 %s\n' % ' '.join([names[mains[1]]] + [names[u] for u in asm] + [names[u] for u in vm]) +
    'rxc82 %s @lib\n' % ' '.join([names[mains[2]]] + [names[u] for u in cp] + [names[mains[3]]]) +
    'lib %s\n' % ' '.join(seen))
print('mknat: %d units, %d files in the tar' % (len(units), len(files)))
EOF
# 4. the PC reference link (the sysroot's cmsrt.o, entry31.o, libraries)
L=$W/usr/lib; P=$O/pc
read -r _ LIBO < <(grep '^lib ' $P/links)
rm -f $P/crxlib.a; (cd $P/o && $X-ar rcs ../crxlib.a $(for n in $LIBO; do echo $n.o; done))
while read -r m objs; do
  [ $m = lib ] && continue
  lib=; case "$objs" in *@lib) lib=$P/crxlib.a; objs=${objs% @lib};; esac
  $X-ld -m elf_s390 -static --emit-relocs --gc-sections -T $L/image.ld -o $P/$m.elf $L/entry31.o \
    $(for n in $objs; do echo $P/o/$n.o; done) $L/cmsrt.o --start-group $lib $L/libc.a $L/libm.a $L/libsoftf.a $L/libgcc.a --end-group
  $E2C $P/$m.elf $P/$m.text >/dev/null
done < $P/links
ls -la $O/crxroot.tar $O/crx82.exec $P/*.text
