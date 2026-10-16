#!/usr/bin/env python3
"""fakedns.py [ADDRESS] -- a stand-in for the PC's name server in tests:
answers every A question with 10.9.9.9 (default address 10.1.4.1:53)."""
import socket, sys
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.bind((sys.argv[1] if len(sys.argv) > 1 else '10.1.4.1', 53))
while True:
    d, a = s.recvfrom(512)
    tid = d[:2]; q = d[12:]; i = 0
    while q[i]: i += q[i] + 1
    qs = q[:i + 5]
    r = tid + b'\x81\x80' + b'\x00\x01\x00\x01\x00\x00\x00\x00' + qs + \
        b'\xc0\x0c\x00\x01\x00\x01\x00\x00\x00\x3c\x00\x04' + bytes([10, 9, 9, 9])
    s.sendto(r, a); print('query from', a, flush=True)
