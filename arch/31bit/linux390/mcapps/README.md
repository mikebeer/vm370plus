# MCproject and MChelpdesk in the Debian guest (9.36, 9.38)

Both are Regina REXX servers on MariaDB (mikebeer/mcproject,
mikebeer/mchelpdesk). On VM/370plus they run unchanged in the 31-bit
Debian 7 guest with:

| Needs | From |
|---|---|
| MariaDB 10.6+ | `linux390/mariadb` (10.11.14, 9.47) |
| Regina 3.9 | `linux390/regina39` (3.9.5 in `/usr/local/regina39`; wheezy's 3.6 stays) |
| perl (Digest::SHA), curl | wheezy packages (`pc/get-wheezy.bat perl curl`) |
| openssl | already on the Debian disk |
| a web front end | `vminetd` here (MCproject's `mcp.rexx --inetd`) |

## MCproject (verified 17 Oct, mc1)

    PATH=/usr/local/regina39/bin:/usr/local/mariadb/bin:$PATH
    cd /opt/mcproject && sh server/bin/mcp-install --operator admin@vm370plus.lan
    chown mcp /etc/mcp /etc/mcp/*; chown -R mcp /var/lib/mcp /var/log/mcp
    install -m 755 vminetd /usr/local/sbin/vminetd
    install -m 755 mcproject.init /etc/init.d/mcproject
    update-rc.d mcproject defaults; /etc/init.d/mcproject start

`curl http://10.1.1.2:8080/api/v1/health` from the PC answers
`{"status":"ok",...}` in about 17 s (Regina reads the 8,400-line program,
then the mariadb client runs once per query).

Wheezy's libcrypt has no yescrypt, so `mcp-crypto` falls back to SHA-512
crypt; with its 400,000 rounds one login took about 10 minutes (10,000
rounds take 15 s). MCproject now has `pw_crypt_rounds` (mikebeer/mcproject
ab8973b, 17 Oct); the guest uses 10000: operator login from the PC in
74 s. `mcp-install` runs under umask 077, so `server/dist` must be made
readable for the `mcp` user (`chmod 755 /opt/mcproject/server/dist`).

## MChelpdesk (verified 17 Oct, mc1)

    bash deploy/install.sh www-data       (PATH with regina39 and mariadb first)
    echo 'pw_hash_iterations = 10000' >> /etc/mchelpdesk/mchelpdesk.conf
    base_url = http://10.1.1.2:8081, cookie_secure = no (plain HTTP on the LAN)
    hd-admin platform-init root root@vm370plus.lan
    install -m 755 vmcgid /usr/local/sbin/vmcgid
    install -m 755 mchelpdesk.init /etc/init.d/mchelpdesk; update-rc.d mchelpdesk defaults

`vmcgid` is a minimal HTTP server for one CGI program (every path is
PATH_INFO). `POST /api/v1/login` from the PC: 200 with a session cookie in
60 s. MChelpdesk's password hash ran a fixed 60,000 SHA-256 rounds in a
MariaDB function (66 s on the guest); it now has `pw_hash_iterations` and
keeps each hash's count in `users.pw_iter` (mikebeer/mchelpdesk d3ab569;
its 150 tests pass).

## vminetd

A one-service inetd in Python 3.2 (wheezy has no systemd socket
activation): `vminetd PORT USER COMMAND...` runs COMMAND per connection with
the socket as stdin and stdout. Two lessons: start-stop-daemon `--background`
leaves fd 0-2 closed, so the children get a real stderr (`VMINETD_LOG`),
or the first file they open becomes fd 2; and an ignored SIGCHLD survives
exec, so the child resets it (Regina's ADDRESS SYSTEM failed with RC -32).
