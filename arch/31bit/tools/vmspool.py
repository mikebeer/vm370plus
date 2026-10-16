#!/usr/bin/env python3
"""vmspool.py -- VM/370plus 9.41: spool a card deck from the PC (Windows,
Linux, macOS) into a VM/370plus user's virtual reader.

    python3 vmspool.py FILE USERID [FN [FT]] [--binary] [--host H] [--port P]

The kit's vm370ce.conf has two socket card readers, which the operator
starts once after IPL (AUTOLOG1 does it):

    010C 3505 127.0.0.1:3505 sockdev ascii trunc eof    text decks
    020C 3505 127.0.0.1:3506 sockdev ebcdic eof         binary decks

A text file goes to 3505 line by line (at most 80 characters a line; longer
lines are cut, and said so).  --binary sends the file as it is, in 80-byte
EBCDIC cards (a TEXT deck, a VMARC or TAPE dump), to 3506; the last card is
padded with X'00'.  Either way an ID card goes first,

    ID USERID NAME FN FT

so CP puts the deck in USERID's reader with that name; FN and FT default to
the file's name and extension.  On CMS:  RDRLIST, or  READCARD FN FT A
(a binary deck: READCARD too, or the program's own receive).
"""
import argparse
import os
import socket
import sys


def card(text, ebcdic):
    text = text[:80].ljust(80)
    return text.encode('cp037') if ebcdic else (text.rstrip() + '\n').encode('ascii', 'replace')


def main():
    ap = argparse.ArgumentParser(description='Spool a deck into a VM/370plus reader.')
    ap.add_argument('file')
    ap.add_argument('userid')
    ap.add_argument('fn', nargs='?')
    ap.add_argument('ft', nargs='?')
    ap.add_argument('--binary', action='store_true', help='80-byte EBCDIC cards, as the file is')
    ap.add_argument('--host', default='127.0.0.1')
    ap.add_argument('--port', type=int)
    a = ap.parse_args()

    base = os.path.basename(a.file)
    stem, ext = os.path.splitext(base)
    fn = (a.fn or stem or 'DECK')[:8].upper()
    ft = (a.ft or ext.lstrip('.') or 'DATA')[:8].upper()
    uid = a.userid[:8].upper()
    idcard = 'ID %s NAME %s %s' % (uid, fn, ft)
    port = a.port or (3506 if a.binary else 3505)

    data = open(a.file, 'rb').read()
    if a.binary:
        if len(data) % 80:
            data += b'\0' * (80 - len(data) % 80)
        payload = card(idcard, True) + data
        n = len(data) // 80
    else:
        lines = data.decode('latin-1').splitlines()
        cut = sum(1 for l in lines if len(l) > 80)
        if cut:
            print('vmspool: %d line(s) longer than 80 characters were cut' % cut, file=sys.stderr)
        payload = card(idcard, False) + b''.join(card(l, False) for l in lines)
        n = len(lines)

    try:
        with socket.create_connection((a.host, port), timeout=30) as s:
            s.sendall(payload)
    except OSError as e:
        sys.exit('vmspool: cannot reach the reader at %s:%d (%s) -- is Hercules up, '
                 'and the reader started (START 10C / 20C)?' % (a.host, port, e))
    print('vmspool: %s -> %s, %s %s, %d card(s)' % (a.file, uid, fn, ft, n))


if __name__ == '__main__':
    main()
