# vmspool — the spool bridge (milestone 9.25)

CMS users call Linux services in the Debian guest the same way they call
the CMS service machines (SQLSERV, CHATBOT): the request is a punch file,
the answer comes back to the reader.

```
CMS user                         Debian guest (MAINT, or the machine set by
                                 GLOBALV LXCALL SERVER)
LXCALL echo hello   --punch-->   vmspoold reads the reader (vmur, I-276 fix)
                                 runs /etc/vmspool.d/echo hello
answer shown,       <--punch--   punches "VMSPOOL RC n" + the output back
kept as LXCALL RESULT A          as VMSPOOL REPLY
```

## Protocol

A request is a punch file. Its first card is

    service FROM userid [argument ...]

and its other cards are the service's input. vmspoold runs the executable
`/etc/vmspool.d/<service>` (in lower case). The arguments go on its command
line, the input goes to stdin, and `VMSPOOL_FROM` and `VMSPOOL_SERVICE` are
set in its environment.

The answer is punched to `userid` as `VMSPOOL REPLY`. Its first card is
`VMSPOOL RC n`. The handler's stdout and stderr follow, and lines longer
than 80 columns go on more cards.
- RC 404 means there is no such service.
- A handler that runs longer than 120 s (`-t`) is killed.

A reader file whose first card is not a request is kept in
`/var/spool/vmspool/unknown/` and gets no answer.

vmspoold reads every file in its reader. If it runs in MAINT, anything else
sent to MAINT's reader is set aside the same way. A dedicated Linux user ID
avoids that.

## Services

| Service | CMS command | What it does |
|---|---|---|
| `echo` | `LXCALL ECHO text (FILE fn ft fm` | Sends the text and the input back (test). |
| `sys` | `LXCALL SYS` | `uname`, `uptime`, disk and memory of the guest. |
| `phone` | `PHONE STATUS`, `CHANNELS`, `PEERS`, `CALL ext [context]`, `HANGUP channel` | Asterisk, through `asterisk -rx`. Arguments are checked: `PHONE CALL 1;reboot` gives `bad argument`, RC 4. |
| `mqtt` | `MQTT PUB topic message`, `MQTT PUB topic (FILE fn ft`, `MQTT SUB topic [n [secs]]` | mosquitto_pub and mosquitto_sub against `MQTT_HOST` (`/etc/vmspool.d/mqtt.conf`, default localhost). |

A new service is any executable dropped into `/etc/vmspool.d`.

## Files

| File | What it is |
|---|---|
| `vmspoold.c` | The daemon. Build: `../../languages/common/wcc -O2 -Wall -o vmspoold vmspoold.c` (wheezy glibc, `-m31`). |
| `vmspoold` | The binary. |
| `vmspoold.init` | `/etc/init.d/vmspoold`. Starts mosquitto first, with `-c /etc/mosquitto/mosquitto.conf`; see the note below. |
| `install.sh` | Installs the daemon, the handlers and the init script, and runs `update-rc.d`. |
| `handlers/` | `echo`, `sys`, `phone`, `mqtt`. |
| `cms/LXCALL.EXEC` | The client. Options: `FILE fn ft fm`, `SERVER userid`, `WAIT n`, `NOTYPE`. Its RC is the service's RC. |
| `cms/PHONE.EXEC`, `cms/MQTT.EXEC` | Short front ends to LXCALL. |

The mosquitto note: started without `-c`, mosquitto drops privileges to a
user `mosquitto` that does not exist. The lookup then kills this build with
`Fatal glibc error: dl-call-libc-early-init`. The config file in the guest
says `user root`.

## Verified (pd15, 13 October)

All calls were made from OPERATOR (CMS) to MAINT (Debian, vmspoold
started by `/etc/init.d/vmspoold`):
- `LXCALL ECHO hello world` answers in 3 s.
- `LXCALL SYS` works.
- `PHONE STATUS` shows Asterisk 1.8.13.1; `PHONE CHANNELS` and `PHONE CALL 100` work.
- `LXCALL NOSUCH` gives RC 404.
- `LXCALL ECHO ... (FILE PROFILE EXEC A` sends the file's input through.
- `MQTT SUB vm/test 1 20` receives a message published on Linux.
- `MQTT PUB vm/out hello from cms` arrives at a mosquitto_sub on Linux.
- A plain `PUNCH PROFILE EXEC` to MAINT is set aside as unknown.

To get the EXECs to CMS, punch them from Linux:

    vmcp spool 00d to operator class a
    awk '{printf "%-80.80s",$0}' LXCALL.EXEC | iconv -f ISO-8859-1 -t IBM1047 \
        | dd obs=8000 of=/dev/vmpun-0.0.000d
    vmcp close 00d name LXCALL EXEC

Write to the punch in multiples of 80 bytes. That is why `dd obs=8000` is
used: iconv's own writes fail.
