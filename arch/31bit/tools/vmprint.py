#!/usr/bin/env python3
"""vmprint.py -- VM/370plus 9.44: CP's printer on the PC (Windows, Linux, macOS).

    python3 vmprint.py OUTDIR [--host H] [--port P] [--idle SECS]
                              [--lp QUEUE] [--pdf] [--once]

The kit's vm370ce.conf has a socket printer,

    010E 1403 127.0.0.1:1403 sockdev

which AUTOLOG1 starts for class P (START 10E CLASS P NOSEP).  vmprint
connects to it (again and again, if Hercules is not up yet or goes away)
and writes every print file that comes to OUTDIR:

    print-YYYYMMDD-HHMMSS.txt   a listing (form feeds kept: one per page)
    print-YYYYMMDD-HHMMSS.ps    PostScript (PSPRINT): the job as it was
                                written on CMS, without CP's padding

A print file ends when the printer has been quiet for --idle seconds
(default 4).  --lp QUEUE prints each file with  lp -d QUEUE  as well
(Windows: --lp is ignored); --pdf makes a .pdf of each PostScript file
when ps2pdf is installed.  On CMS:  PCPRINT fn ft fm  (or CP SPOOL PRINTER
CLASS P, then PRINT).  If vmprint was not connected when CP started the
printer, CP stops it (intervention required); START 10E again from the
operator once vmprint runs.
"""
import argparse
import os
import shutil
import socket
import subprocess
import sys
import time


def ps_jobs(text):
    """the PostScript jobs in a listing (as psprint.py), and the lines
    outside them (other print files that came in the same stretch)"""
    out, cur, rest = [], None, []
    for raw in text.split('\n'):
        line = raw.rstrip(' \r\f')
        if cur is None:
            if line.lstrip('\f').startswith('%!PS-Adobe') and 'Resource' not in line:
                cur = [line.lstrip('\f')]
            else:
                rest.append(raw)
        else:
            cur.append(line)
            if line.startswith('%%EOF'):
                out.append('\n'.join(cur) + '\n')
                cur = None
    return out, rest


def save(outdir, data, a):
    # what is not text: Hercules' printer sends CR for overprinting and
    # bytes for codes that have no ASCII character
    text = ''.join(ch for ch in data.decode('latin-1')
                   if ch in '\n\f' or ' ' <= ch <= '~' or ch >= '\xa0')
    stamp = time.strftime('%Y%m%d-%H%M%S')
    files = []
    jobs, rest = ps_jobs(text)
    for i, j in enumerate(jobs):
        p = os.path.join(outdir, 'print-%s%s.ps' % (stamp, '-%d' % (i + 1) if len(jobs) > 1 else ''))
        open(p, 'w', encoding='latin-1').write(j)
        files.append(p)
        if a.pdf and shutil.which('ps2pdf'):
            subprocess.call(['ps2pdf', p, p[:-3] + '.pdf'])
    lines = [l.rstrip(' ') for l in rest]
    while lines and not lines[-1].strip('\f '):
        lines.pop()
    if lines:
        p = os.path.join(outdir, 'print-%s.txt' % stamp)
        open(p, 'w', encoding='latin-1', newline='\n').write('\n'.join(lines) + '\n')
        files.append(p)
    for p in files:
        print('vmprint: %s (%d bytes)' % (p, os.path.getsize(p)), flush=True)
        if a.lp and os.name != 'nt' and not p.endswith('.pdf'):
            subprocess.call(['lp', '-d', a.lp, p])
    return files


def main():
    ap = argparse.ArgumentParser(description="CP's socket printer (010E) on the PC.")
    ap.add_argument('outdir')
    ap.add_argument('--host', default='127.0.0.1')
    ap.add_argument('--port', type=int, default=1403)
    ap.add_argument('--idle', type=float, default=4.0)
    ap.add_argument('--lp')
    ap.add_argument('--pdf', action='store_true')
    ap.add_argument('--once', action='store_true', help='stop after the first print file')
    a = ap.parse_args()
    os.makedirs(a.outdir, exist_ok=True)
    said = refused = False
    while True:
        try:
            s = socket.create_connection((a.host, a.port), timeout=10)
        except OSError:
            if not said:
                print('vmprint: waiting for the printer at %s:%d (is Hercules up?)' % (a.host, a.port), flush=True)
                said = True
            time.sleep(5)
            continue
        t0 = time.time()
        said = False
        s.settimeout(a.idle)
        buf = b''
        got = announced = False
        while True:
            try:
                d = s.recv(65536)
            except socket.timeout:
                if not announced:   # the connection held: the printer is ours
                    print('vmprint: connected to %s:%d, print files go to %s'
                          % (a.host, a.port, a.outdir), flush=True)
                    announced, refused = True, False
                if buf:
                    save(a.outdir, buf, a)
                    buf = b''
                    if a.once:
                        return
                continue
            except OSError:
                d = b''
            if not d:
                break
            got = True
            buf += d
        if buf:
            save(a.outdir, buf, a)
            if a.once:
                return
        s.close()
        if time.time() - t0 < 2 and not got:
            # Hercules took the connection and closed it: another client
            # has the printer, or it is busy -- try again later
            if not refused:
                print('vmprint: the printer at %s:%d refuses (another vmprint connected?); '
                      'trying again' % (a.host, a.port), flush=True)
                refused = True
            time.sleep(10)
            continue
        print('vmprint: the printer went away; waiting for it again', flush=True)


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        pass
