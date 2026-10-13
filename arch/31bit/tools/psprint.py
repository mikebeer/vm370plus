#!/usr/bin/env python3
"""psprint.py -- VM/370plus 9.31, the PC half of graphical printing by spool.

A CMS user prints a PostScript file on the virtual printer (PSPRINT EXEC, or
PRINT fn PS A (LINECOUNT 0); CP's real printer 00E writes it to the PC file
of the Hercules 1403 (io/print1.listing).  This takes every PostScript job
out of that listing -- from a line '%!PS-Adobe' to its '%%EOF' -- without
CP's separator pages or the trailing blanks of the 132-column lines, and
writes each one as a .ps file; it can hand each to a printer as well.

    psprint.py LISTING OUTDIR [--from BYTE] [--follow]
               [--lp QUEUE | --raw HOST[:9100] | --pdf]

  --from BYTE   start at this offset of the listing (the listing grows;
                OUTDIR/.offset remembers where the last run stopped)
  --follow      keep watching the listing
  --lp QUEUE    print each job with  lp -d QUEUE
  --raw HOST    send each job to a printer's raw port (9100)
  --pdf         also make job.pdf with ps2pdf, when it is installed

Verified 13 Oct (pd19): groff's HELLO PS from CMS came out byte for byte.
"""
import os
import re
import shutil
import socket
import subprocess
import sys
import time


def jobs(text):
    """the PostScript jobs in a piece of the listing"""
    out, cur = [], None
    for line in text.split('\n'):
        line = line.rstrip(' \r\f')
        if cur is None:
            if line.startswith('%!PS-Adobe') and 'Resource' not in line:
                cur = [line]
        else:
            cur.append(line)
            if line.startswith('%%EOF'):
                out.append('\n'.join(cur) + '\n')
                cur = None
    return out, cur is not None


def deliver(path, opts):
    if opts.get('lp'):
        subprocess.call(['lp', '-d', opts['lp'], path])
    if opts.get('raw'):
        host, _, port = opts['raw'].partition(':')
        with socket.create_connection((host, int(port or 9100)), 30) as s:
            s.sendall(open(path, 'rb').read())
    if opts.get('pdf') and shutil.which('ps2pdf'):
        subprocess.call(['ps2pdf', path, path[:-3] + '.pdf'])


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 2
    listing, outdir = argv[0], argv[1]
    opts = {'follow': '--follow' in argv, 'pdf': '--pdf' in argv}
    for k in ('--lp', '--raw', '--from'):
        if k in argv:
            opts[k[2:]] = argv[argv.index(k) + 1]
    os.makedirs(outdir, exist_ok=True)
    mark = os.path.join(outdir, '.offset')
    pos = int(opts['from']) if 'from' in opts else (
        int(open(mark).read()) if os.path.exists(mark) else 0)
    n = 0
    while True:
        data = open(listing, 'rb').read()
        if len(data) < pos:          # a new listing
            pos = 0
        piece = data[pos:].decode('latin-1')
        found, open_job = jobs(piece)
        if found:
            for j in found:
                n += 1
                name = os.path.join(outdir, time.strftime('job-%Y%m%d-%H%M%S') + '-%d.ps' % n)
                open(name, 'w', encoding='latin-1').write(j)
                print('psprint: %s (%d lines)' % (name, j.count('\n')))
                deliver(name, opts)
            # resume after the last complete job
            end = piece.rfind('%%EOF')
            pos += len(piece[:end].encode('latin-1')) + 5
            open(mark, 'w').write(str(pos))
        if not opts['follow']:
            break
        time.sleep(2)
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
