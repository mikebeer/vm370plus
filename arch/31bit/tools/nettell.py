#!/usr/bin/env python3
"""nettell.py -- VM/370plus 9.32: TELL from anywhere on the internet.

    nettell.py [--tls [CAFILE]] HOST[:PORT] LOGIN PASSWORD listen
    nettell.py [--tls [CAFILE]] HOST[:PORT] LOGIN PASSWORD send USERID message ...

--tls: the PC runs netfwd.py --tls (port 8883); CAFILE is the certificate
to trust (for a self-signed one, the PC's vm.crt itself).

`send` reaches a VM/370 user as  MSG FROM MAINT: (net:LOGIN) message.
`listen` shows what CMS users send with  TELL LOGIN AT NET message
(and the gateway's notes, such as a user who is not logged on).
HOST is the PC that forwards to the guest's broker (netfwd.py), or the
guest itself (10.1.1.2) on the PC.  Any MQTT client does the same:
publish to vm/tell/from/LOGIN/to/USERID, subscribe to vm/tell/out/LOGIN.
The login comes from  tellnet-user LOGIN PASSWORD  in the Debian guest.
Needs paho-mqtt (pip install paho-mqtt).
"""
import sys, time
import paho.mqtt.client as mqtt


def main(a):
    tls = None
    if a[:1] == ['--tls']:
        a = a[1:]
        tls = ''
        if a and (a[0].endswith('.crt') or a[0].endswith('.pem')):
            tls, a = a[0], a[1:]
    if len(a) < 4 or a[3] not in ('listen', 'send') or (a[3] == 'send' and len(a) < 6):
        print(__doc__); return 2
    host, _, port = a[0].partition(':')
    login, pw, cmd = a[1], a[2], a[3]
    c = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id='nettell-' + login + ('-s%d' % (time.time() * 1000 % 100000) if cmd == 'send' else ''),
                    clean_session=(cmd == 'send'))
    c.username_pw_set(login, pw)
    if tls is not None:
        c.tls_set(ca_certs=tls or None)
    state = {}
    c.on_connect = lambda cl, u, f, rc, p=None: state.update(rc=rc)
    c.on_message = lambda cl, u, m: print(m.payload.decode('latin-1'), flush=True)
    c.connect(host, int(port or (8883 if tls is not None else 1883)), 30)
    c.loop_start()
    for _ in range(100):
        if 'rc' in state: break
        time.sleep(0.1)
    if state.get('rc') != 0:
        print('nettell: not connected:', state.get('rc', 'no answer')); return 8
    if cmd == 'send':
        r = c.publish('vm/tell/from/%s/to/%s' % (login, a[4]), ' '.join(a[5:]), qos=1)
        r.wait_for_publish(30)
        print('nettell: sent to', a[4].upper())
    else:
        c.subscribe('vm/tell/out/' + login, qos=1)
        try:
            while True: time.sleep(1)
        except KeyboardInterrupt:
            pass
    c.loop_stop(); c.disconnect()
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
