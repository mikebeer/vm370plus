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
| `troff` | `TROFF fn [ft [fm]] (MACROS ms PS` | groff 1.21 (groff-base) with the ms, me, mm and mom macros of groff 1.23 (`/usr/local/share/groff/tmac123`). Plain text comes back as `fn DOCUMENT A`; with PS, PostScript comes back as `fn PS A`, and a copy stays in `/var/spool/vmspool/out/<user>/`. (9.28) |
| `markdown` | `MDPRINT fn [ft [fm]] (PS` | (9.42) Markdown through `md2ms` (Markdown to troff ms with tbl tables) and `groff -t`: PostScript comes back as `fn PS A` (then `PSPRINT`), a copy stays in `/var/spool/vmspool/out/<user>/`. `MDPRINT` without PS lays the text out on CMS itself (TYPE, DISK, PRINT). On Debian: `mdprint FILE [-w N \| -ps [OUT] \| -pdf [OUT]]`. |
| `mail` | `SENDMAIL fn [ft [fm]]` | (9.18) The note (To:, Cc:, Bcc:, Subject:, an empty line, the text) as a mail from `userid@DOMAIN`: CMS users get it in their reader at once (`GETMAIL`), other addresses go out by `SMARTHOST` or the domain's MX (`/etc/vmspool.d/mail.conf`). |
| `ask` | `ASK question ... (FILE fn ft fm`, `ASK NEW` | (9.10) The AI gateway: the question (and the file) goes to the model `/etc/vmspool.d/ask.conf` names -- an OpenAI-compatible server (llama.cpp's llama-server, Ollama, LM Studio on the PC at 10.1.1.1; OpenAI) or Anthropic's API -- with the user's last HISTORY exchanges; the answer comes back in lines of 79. `ask` on Linux does the same. |
| `tell` | `TELL user AT LINUX message` | (9.14) The message (sent as input cards, so any text is safe) is written to the Linux user's terminals and kept in `/var/spool/vmspool/msg/<user>`; `ALL` reaches everybody logged on. RC 1 when the user is not logged on. The other way, `tell userid message` on Linux sends `CP MSG userid (linuxuser) message` through the setuid helper `vmmsg` (only MSG, user ID checked); `tell` alone shows the kept messages. `TELL userid message` without `AT` is CP MSG. |
| `mqtt` | `MQTT PUB topic message`, `MQTT PUB topic (FILE fn ft`, `MQTT SUB topic [n [secs]]` | mosquitto_pub and mosquitto_sub against `MQTT_HOST` (`/etc/vmspool.d/mqtt.conf`, default localhost). |

A new service is any executable dropped into `/etc/vmspool.d`.

## Files

| File | What it is |
|---|---|
| `vmspoold.c` | The daemon; since 9.18 it also punches the files other programs leave in `<spool>/outq` (first line `TO userid [CLASS c] [NAME fn ft]`), so that it alone uses the punch. Build: `../../languages/common/wcc -O2 -Wall -o vmspoold vmspoold.c` (wheezy glibc, `-m31`). |
| `vmspoold` | The binary. |
| `vmspoold.init` | `/etc/init.d/vmspoold`. Starts mosquitto first, with `-c /etc/mosquitto/mosquitto.conf`; see the note below. |
| `install.sh` | Installs the daemon, the handlers and the init script, and runs `update-rc.d`. |
| `handlers/` | `echo`, `sys`, `phone`, `mqtt`. |
| `cms/LXCALL.EXEC` | The client. Options: `FILE fn ft fm`, `SERVER userid`, `WAIT n`, `NOTYPE`. Its RC is the service's RC. |
| `cms/PHONE.EXEC`, `cms/MQTT.EXEC`, `cms/TELL.EXEC` | Short front ends to LXCALL. |
| `linux/vmmail.py`, `linux/vmmaild`, `linux/vmmaild.init`, `linux/mail.conf` | (9.18) Mail: `vmmaild` takes SMTP on port 25 and delivers `userid@DOMAIN` to that user's reader as `sender MAIL` (through vmspoold's outbox); mail for other domains only from 10.x/127.x (no open relay). `vmmail.py` (in `/usr/local/lib/vmspool`) is shared with the `mail` service. Mail for the Linux machine's own userid stays in `/var/spool/vmspool/mail/<user>/`, where a copy of every message is kept. |
| `cms/SENDMAIL.EXEC`, `cms/GETMAIL.EXEC` | (9.18) Mail from CMS (over LXCALL) and from the reader into `MAILnnnn MAIL A` files. |
| `linux/vmask.py`, `linux/ask`, `linux/ask.conf`, `cms/ASK.EXEC` | (9.10) The AI gateway: the client library, the Linux command, the configuration, the CMS command. `tools/fakellm.py` is a stand-in model server for tests. |
| `linux/tell`, `linux/vmmsg.c` | The Linux half of TELL (9.14); `vmmsg` is installed setuid root. |

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

## Printing PostScript (9.31, by spool)

`PSPRINT fn [ft [fm]]` checks that the file begins with `%!PS` and runs
`PRINT ... (LINECOUNT 0`, so CMS adds no page headers. CP's printer 00E
then writes the file to the PC's 1403 file (`io/print1.listing`). On the
PC, `tools/psprint.py LISTING OUTDIR [--follow] [--lp QUEUE | --raw
HOST:9100 | --pdf]` does the rest:
- takes each PostScript job out (from `%!PS-Adobe` to `%%EOF`), without
  CP's separator pages or the blanks that pad the lines;
- writes each job as a `.ps` file;
- sends it to a printer or makes a PDF.

Lines must be at most 132 characters; groff's PostScript lines are at most
80.

Verified (pd19, 13 October): `TROFF HELLO (MACROS MS PS` and then `PSPRINT
HELLO`. The PostScript on the PC matched the original in the Debian guest
byte for byte, and Ghostscript rendered it and turned it into a PDF.

## Printing by bridge (9.31)

`PSPRINT fn [ft [fm]] (BRIDGE [format]` sends the PostScript to the
bridge's `print` service. Ghostscript (9.29) converts it to the printer's
format — `ps`, `pdf`, `ljet4`, `pxlmono` or `pxlcolor` — and the service
sends the result to `PRINTER=host[:9100]` (the raw port) or to
`LPQUEUE=name` (lp). Both settings live in `/etc/vmspool.d/print.conf`,
along with `FORMAT=`.

The Debian guest's network goes through the PC (10.1.1.1). A printer on
the LAN therefore needs routing or NAT on the PC, or a forward from the
PC's port 9100 to the printer.

Verified (pd19): a 12,491-byte PDF arrived at a stand-in printer on the
PC's port 9100.
