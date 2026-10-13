# TELL to and from the internet (milestone 9.32)

TELL (9.14) reaches VM/370 users and users of the Debian guest. 9.32 adds
users anywhere on the internet, through the MQTT broker (mosquitto) that
already runs in the guest. This is the interim route until the BORDER
firewall (9.12c).

```
CMS:  TELL bob AT NET hello      -> vmspoold "tellnet" -> MQTT vm/tell/out/bob
bob:  any MQTT client, or tools/nettell.py ... listen  <- reads vm/tell/out/bob

bob:  publish vm/tell/from/bob/to/cmsuser "hi"         -> tellgw (guest)
CMS:  MSG FROM MAINT: (net:bob) hi                     <- CP MSG via vmcp
```

## Security

- **Two listeners.** The broker in the guest has a local listener
  (127.0.0.1, no login) for CMS, the gateway and Linux programs. It has an
  outside listener (10.1.1.2) that requires a login.
- **Logins.** `tellnet-user NAME PASSWORD` adds an outside user,
  `tellnet-user -d NAME` removes one, and `-l` lists them.
- **ACL.** An outside user may publish only to `vm/tell/from/<own login>/to/+`
  and read only `vm/tell/out/<own login>`. The sender name the CMS user sees
  is therefore the broker's login and cannot be forged; a test that
  published as someone else was dropped.
- **Plain-text passwords.** This mosquitto is built without OpenSSL, so it
  keeps passwords in plain text in `/etc/mosquitto/passwd` (mode 0600,
  root only).
- **Encryption.** The PC side encrypts the link: `tools/netfwd.py --tls
  CERT KEY 8883 10.1.1.2:1883`.

## The PC side

The guest's network ends at the PC. `tools/netfwd.py` forwards a PC port to
the broker:

- On a LAN: `netfwd.py 1883 10.1.1.2:1883`.
- On the internet: `netfwd.py --tls vm.crt vm.key 8883 10.1.1.2:1883`.
  The router must also forward port 8883 to the PC.

A self-signed certificate is made with:

    openssl req -x509 -newkey rsa:2048 -nodes -keyout vm.key -out vm.crt \
        -days 365 -subj "/CN=your.host.name"

`tools/nettell.py` is a small outside client (paho-mqtt):

    nettell.py --tls vm.crt your.host.name bob PASSWORD listen
    nettell.py --tls vm.crt your.host.name bob PASSWORD send cmsuser hello

## Files

| File | Where it goes |
|---|---|
| `tellnet.handler` | `/etc/vmspool.d/tellnet`, the bridge service for `TELL user AT NET` |
| `tellgw`, `tellgw.init` | `/usr/local/sbin/tellgw`, `/etc/init.d/tellgw`; the gateway subscribes again when the broker restarts |
| `tellnet-user` | `/usr/local/sbin/tellnet-user` |
| `mosquitto.conf`, `acl`, `tellnet.conf` | `/etc/mosquitto/`, `/etc/vmspool.d/` |
| `install.sh` | installs all of the above (as root, in this directory) |

## Verified (pd24, 13 October)

All tests were run with bob on the PC, through netfwd.

- **CMS to bob:** `TELL bob AT NET Hello Bob, ...` arrived at bob's
  listener.
- **bob to CMS:** bob's `send operator ...` arrived on OPERATOR's console
  as `MSG FROM MAINT: (net:bob) ...`.
- **User not logged on:** a message to `cmsuser` got the answer
  `tell: CMSUSER is not logged on`.
- **Rejected:** a wrong password, and a message published under another
  login.
- **TLS:** both directions worked again over `netfwd.py --tls` with a
  self-signed certificate.
