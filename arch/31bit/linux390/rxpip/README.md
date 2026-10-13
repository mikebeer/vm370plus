# RXPIP on VM/370plus (milestone 9.26)

[mikebeer/rexxpository](https://github.com/mikebeer/rexxpository) is the
RXPIP REXX package manager. On VM/370plus it runs in the Debian guest:
- the repository server, `rxpip_server.rexx`, under Regina;
- the `rxpip` client;
- a spool bridge service (9.25), through which CMS users get the same
  packages.

```
CMS:  RXPIP RUN DEMO                     Debian guest
      DEMO REXX (with  REQUIRE pkg )  -> vmspoold -> /etc/vmspool.d/rxpip
                                         rxpip build demo.rexx (installs the
                                         packages from the local repository)
      DEMO EXEC A  <-  the merged, standalone program
      runs it
```

## CMS commands (RXPIP EXEC)

| Command | Result |
|---|---|
| `RXPIP AVAIL` | Every package in the repository. |
| `RXPIP SEARCH term`, `RXPIP SHOW name` | Asks the repository. |
| `RXPIP BUILD fn` | Takes `fn REXX *`, a program with `/* REQUIRE pkg */` comments, and writes `fn EXEC A` with its packages merged in, as `rxpip build` does. |
| `RXPIP RUN fn [args]` | BUILD, then runs `fn EXEC`. |
| `RXPIP GET name` | Fetches the package's source files as `file REXX A`. |

Verified in pd19 (13 October), as OPERATOR:
- `RXPIP RUN DEMO` prints `HELLO, WORLD!!!`, `square of 7 is 49`,
  `cube of 3 is 27` and `17 is prime`.
- `RXPIP GET STRUTILS` and `RXPIP SEARCH MATH` work.

**Speed.** On the emulated machine every repository request costs a few
seconds, and a first `BUILD` that installs three packages takes minutes. The
bridge timeout is 900 s.

## Debian side

`setup.sh`, run as root in a directory that also holds a copy of
rexxpository, installs the following:
- `/opt/rxpip`: the server, the client and the examples.
- `rxpipd.py`: rexxpository's `tests/cgi_harness.py`, made to run on
  wheezy's Python 3.2. It is started by `/etc/init.d/rxpipd` and listens
  on `127.0.0.1:8089`; the repository is `/var/lib/rxpip/repo`.
- `/usr/local/bin/rxpip` (Regina) and `/usr/local/bin/curl`. The guest has
  no curl, so the second one is `minicurl.py`, which does what rxpip asks
  of curl (`-s -f -o -X -H` including `-H @file`, `--data-binary @file`).
- The example packages (mathutils, strutils, greetutils), published by the
  user `vm370`.
- The vmspoold service `rxpip` (`rxpip.handler`).

## Notes

- **rexxpository bug.** `rxpip build` failed for a program with REQUIREs
  ("Incorrect call to routine") because `CmdBuild` did not expose `cfg.`.
  The one-word fix is on branch `fix-build-cfg` of mikebeer/rexxpository;
  our copy carries it.
- **I-277 (fixed 13 October).** A REXX EXEC called with 8 or more
  arguments ended CMS in machines that load the GCCLIB segment, because the
  segment had been saved from an older GCCLIB. The segment has been saved
  again. RXPIP still hands LXCALL its options in `LXCALL OPTIONS A`.
- **Not done yet.** Publishing from CMS (`RXPIP PUBLISH`) and an external
  repository (www.rexxpository.org). The guest's network reaches only the
  PC.

## Updates and private packages

The handler sets `RXPIP_NO_UPDATE_CHECK=1`, so the client's daily "a newer
version is available" hint never reaches a CMS user's reader. Update the
bridge on the Debian side: `cd /opt/rxpip && git pull` (or `rxpip update`
as the owner of `/opt/rxpip/client`).

Private packages need a token. The bridge uses one service-level token, of
a dedicated non-admin repository user (for example `cmsbridge`, created by
the repository admin with `admin adduser cmsbridge` and added as a reader of
each private package CMS should see: `rxpip reader add <pkg> cmsbridge`).
Once, on the Debian guest:

    RXPIP_HOME=/var/lib/rxpip/client regina -a /opt/rxpip/client/rxpip.rexx login --token=<token>
    chmod 600 /var/lib/rxpip/client/credentials.txt

SEARCH, SHOW, GET and BUILD then pick the token up from the credentials
file; every CMS user sees the same set. Against a closed repository
(`RXPIP_CLOSED=1` on the server) the bridge needs the token for everything,
AVAIL included. Per-user visibility would need a token per CMS user
(`RXPIP_TOKEN` per call); not done.
