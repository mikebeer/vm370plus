#!/usr/bin/env python3
"""vmspool.py -- VM/370plus 9.41: spool a card deck from the PC (Windows,
Linux, macOS) into a VM/370plus user's virtual reader.

    python3 vmspool.py FILE USERID [FN [FT]] [--binary] [--host H] [--port P]

The kit's vm370ce.conf has two socket card readers, which the operator
starts once after IPL (AUTOLOG1 does it):

    010C 3505 127.0.0.1:3505 sockdev ascii trunc eof    text decks
    020C 3505 127.0.0.1:3506 sockdev ebcdic eof         binary decks

A text file goes to 3505 line by line.  One with a line longer than 80
characters goes as a stream instead (--cut: cut them; --stream: always):
the lines with X'15' after each, in 80-byte cards through 3506; READCARD,
then UNSTREAM fn ft on CMS makes the lines again (a V file).  --binary sends the file as it is, in 80-byte
EBCDIC cards (a TEXT deck, a VMARC or TAPE dump), to 3506; the last card is
padded with X'00'.  Either way an ID card goes first,

    ID USERID NAME FN FT

so CP puts the deck in USERID's reader with that name; FN and FT default to
the file's name and extension.  On CMS:  RDRLIST, or  READCARD FN FT A
(a binary deck: READCARD too, or the program's own receive).
"""
import argparse
import binascii
import os
import socket
import sys


# Hercules' ASCII to EBCDIC table for CODEPAGE 819/1047 (vm370ce.conf), so
# a deck sent here reads as the same characters as one through an ascii
# card reader
A2E = binascii.unhexlify(
    '00010203372D2E2F1605250B0C0D0E0F101112133C3D322618193F271C1D1E1F405A7F7B5B6C507D4D5D5C4E6B604B61F0F1F2F3F4F5F6F7F8F97A5E4C7E6E6F7CC1C2C3C4C5C6C7C8C9D1D2D3D4D5D6D7D8D9E2E3E4E5E6E7E8E9ADE0BD5F6D79818283848586878889919293949596979899A2A3A4A5A6A7A8A9C04FD0A107202122232415061728292A2B2C090A1B30311A333435360838393A3B04143EFF41AA4AB19FB26AB5BBB49A8AB0CAAFBC908FEAFABEA0B6B39DDA9B8BB7B8B9AB6465626663679E687471727378757677AC69EDEEEBEFECBF80FDFEFBFCBAAE594445424643479C4854515253585556578C49CDCECBCFCCE170DDDEDBDC8D8EDF')


# UTF-8 text: what CMS has no character for, in ASCII
UMAP = {'\u2014': '--', '\u2013': '-', '\u2018': "'", '\u2019': "'", '\u201c': '"',
        '\u201d': '"', '\u2026': '...', '\u2022': '*', '\u00a0': ' ', '\u2192': '->',
        '\u2190': '<-', '\u2264': '<=', '\u2265': '>=', '\u00d7': 'x', '\u2212': '-'}


def text_of(data):
    try:
        t = data.decode('utf-8')
    except UnicodeDecodeError:
        return data.decode('latin-1')
    t = ''.join(UMAP.get(c, c) for c in t)
    return t.encode('latin-1', 'replace').decode('latin-1')


def a2e(text):
    return bytes(A2E[b] for b in text.encode('latin-1', 'replace'))


def card(text, ebcdic):
    text = text[:80].ljust(80)
    return a2e(text) if ebcdic else (text.rstrip() + '\n').encode('ascii', 'replace')


def main():
    ap = argparse.ArgumentParser(description='Spool a deck into a VM/370plus reader.')
    ap.add_argument('file')
    ap.add_argument('userid')
    ap.add_argument('fn', nargs='?')
    ap.add_argument('ft', nargs='?')
    ap.add_argument('--binary', action='store_true', help='80-byte EBCDIC cards, as the file is')
    ap.add_argument('--stream', action='store_true',
                    help='a text file as a stream (automatic for lines over 80): UNSTREAM fn ft on CMS')
    ap.add_argument('--cut', action='store_true', help='cut lines over 80 instead of streaming')
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
    stream = False
    if not a.binary:
        lines = text_of(data).splitlines()
        stream = a.stream or (not a.cut and any(len(l) > 80 for l in lines))
    if stream:
        # the lines one after the other, X'15' (EBCDIC NL) after each, in
        # 80-byte cards padded with X'00', through the binary reader
        body = b''.join(a2e(l.rstrip()) + b'\x15' for l in lines)
        if len(body) % 80:
            body += b'\0' * (80 - len(body) % 80)
        payload = card(idcard, True) + body
        port = a.port or 3506
        n = len(lines)
    elif a.binary:
        if len(data) % 80:
            data += b'\0' * (80 - len(data) % 80)
        payload = card(idcard, True) + data
        n = len(data) // 80
    else:
        lines = text_of(data).splitlines()
        cut = sum(1 for l in lines if len(l) > 80)
        if cut:
            print('vmspool: %d line(s) longer than 80 characters were cut' % cut, file=sys.stderr)
        payload = card(idcard, False) + b''.join(
            (l[:80].rstrip() + '\n').encode('latin-1', 'replace') for l in lines)
        n = len(lines)

    try:
        with socket.create_connection((a.host, port), timeout=30) as s:
            s.sendall(payload)
    except OSError as e:
        sys.exit('vmspool: cannot reach the reader at %s:%d (%s) -- is Hercules up, '
                 'and the reader started (START 10C / 20C)?' % (a.host, port, e))
    if stream:
        print('vmspool: %s -> %s, %s %s, %d line(s) as a stream: on CMS  READCARD %s %s A  then  UNSTREAM %s %s'
              % (a.file, uid, fn, ft, n, fn, ft, fn, ft))
    else:
        print('vmspool: %s -> %s, %s %s, %d card(s)' % (a.file, uid, fn, ft, n))


if __name__ == '__main__':
    main()
