#!/usr/bin/env python3
"""netfwd.py -- forward a TCP port of the PC to the Debian guest (9.32).

    netfwd.py [--tls CERT KEY] [ADDR:]PORT TARGET:PORT
        e.g.  netfwd.py --tls vm.crt vm.key 8883 10.1.1.2:1883

With --tls the PC end speaks TLS (MQTT over TLS is port 8883), so logins
and messages cross the internet encrypted; the stretch PC -> guest stays
inside the PC.  Without it (a LAN only): netfwd.py 1883 10.1.1.2:1883.

The guest's network ends at the PC (10.1.1.1 <-> 10.1.1.2), so a client on
the LAN or the internet reaches the guest's MQTT broker (TELL AT NET) only
through a forward on the PC; on the internet, the router must forward the
port to the PC as well.  The broker asks outside users for a login.
"""
import asyncio, ssl, sys


async def pipe(r, w):
    try:
        while True:
            d = await r.read(65536)
            if not d: break
            w.write(d); await w.drain()
    except Exception:
        pass
    finally:
        w.close()


async def main(listen, target, ctx=None):
    la, _, lp = listen.rpartition(':')
    th, _, tp = target.partition(':')

    async def handle(r, w):
        try:
            tr, tw = await asyncio.open_connection(th, int(tp))
        except OSError as e:
            print('netfwd:', e, flush=True); w.close(); return
        print('netfwd: %s -> %s' % (w.get_extra_info('peername'), target), flush=True)
        await asyncio.gather(pipe(r, tw), pipe(tr, w))

    srv = await asyncio.start_server(handle, la or '0.0.0.0', int(lp), ssl=ctx)
    print('netfwd: %s -> %s%s' % (listen, target, ' (TLS)' if ctx else ''), flush=True)
    async with srv:
        await srv.serve_forever()


if __name__ == '__main__':
    a = sys.argv[1:]
    ctx = None
    if a[:1] == ['--tls'] and len(a) >= 3:
        ctx = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH)
        ctx.load_cert_chain(a[1], a[2])
        a = a[3:]
    if len(a) != 2:
        print(__doc__); sys.exit(2)
    asyncio.run(main(a[0], a[1], ctx))
