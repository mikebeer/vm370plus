#!/usr/bin/env python3
"""dnsq.py SERVER[:PORT] NAME [TYPE] -- one DNS question, the answer decoded
(A, PTR, MX, NS, SOA, CNAME); for testing vmdns (9.40)."""
import random, socket, struct, sys
T = {'A': 1, 'NS': 2, 'CNAME': 5, 'SOA': 6, 'PTR': 12, 'MX': 15, 'AAAA': 28, 'ANY': 255}
R = {v: k for k, v in T.items()}
def name(p, o):
    out = []; jumped = False; end = o
    while True:
        c = p[o]
        if c >= 0xC0:
            if not jumped: end = o + 2
            o = (c & 0x3F) << 8 | p[o + 1]; jumped = True; continue
        if c == 0:
            if not jumped: end = o + 1
            return '.'.join(out), end
        out.append(p[o + 1:o + 1 + c].decode('latin-1')); o += c + 1
def main():
    srv, n = sys.argv[1], sys.argv[2]
    t = sys.argv[3].upper() if len(sys.argv) > 3 else 'A'
    host, _, port = srv.partition(':')
    tid = random.randint(0, 65535)
    q = struct.pack('>HHHHHH', tid, 0x0100, 1, 0, 0, 0) + b''.join(
        bytes([len(l)]) + l.encode() for l in n.split('.') if l) + b'\0' + struct.pack('>HH', T[t], 1)
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM); s.settimeout(5)
    s.sendto(q, (host, int(port or 53)))
    p, _ = s.recvfrom(1500)
    rid, fl, qd, an, ns, ar = struct.unpack('>HHHHHH', p[:12])
    print('id %s rcode %d aa %d answers %d additional %d' % (rid == tid, fl & 15, fl >> 10 & 1, an, ar))
    o = 12
    for _ in range(qd): _, o = name(p, o); o += 4
    for _ in range(an + ns + ar):
        nm, o = name(p, o)
        ty, cl, ttl, rl = struct.unpack('>HHIH', p[o:o + 10]); o += 10
        rd = p[o:o + rl]
        if ty == 1: v = socket.inet_ntoa(rd)
        elif ty in (2, 5, 12): v = name(p, o)[0]
        elif ty == 15: v = '%d %s' % (struct.unpack('>H', rd[:2])[0], name(p, o + 2)[0])
        elif ty == 6: v = name(p, o)[0]
        else: v = rd.hex()
        print('%-30s %-5s %s' % (nm, R.get(ty, ty), v)); o += rl
main()
