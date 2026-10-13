#!/bin/sh
# setup.sh -- RXPIP (9.26) on the Debian guest, as root, in this directory
# (rexxpository/ = a copy of mikebeer/rexxpository):
#   the server (rxpip_server.rexx under rxpipd.py, port 8089, local only),
#   the client as  rxpip  (Regina), the example packages published by the
#   user  vm370 , and the vmspoold service  rxpip  for CMS (RXPIP EXEC).
set -e
mkdir -p /opt/rxpip /var/lib/rxpip/repo /var/lib/rxpip/client
# the guest has no curl: minicurl does what rxpip asks of it
command -v curl >/dev/null || install -m 755 minicurl.py /usr/local/bin/curl
cp -r rexxpository/server rexxpository/client rexxpository/examples /opt/rxpip/
install -m 755 rxpipd.py /opt/rxpip/rxpipd.py
install -m 755 rxpipd.init /etc/init.d/rxpipd
update-rc.d rxpipd defaults >/dev/null
cat > /usr/local/bin/rxpip <<'W'
#!/bin/sh
exec regina -a /opt/rxpip/client/rxpip.rexx "$@"
W
chmod 755 /usr/local/bin/rxpip
/etc/init.d/rxpipd start
URL=http://127.0.0.1:8089/cgi-bin/rxpip_server.rexx
for i in 1 2 3 4 5 6 7 8 9 10; do
  curl -s "$URL?action=PING" | grep -q "STATUS: OK" && break; sleep 1
done
for h in /root/.rxpip /var/lib/rxpip/client; do
  RXPIP_HOME=$h rxpip config set REPO_URL $URL >/dev/null
done
[ -d /etc/vmspool.d ] && install -m 755 rxpip.handler /etc/vmspool.d/rxpip
# the examples, published once by the user vm370
if ! curl -s "$URL?action=LIST" | grep -q mathutils; then
  # the publishing user: vm370, or vm370-n when an earlier run made it
  for u in vm370 vm370-2 vm370-3 vm370-4 vm370-5; do
    o=$(RXPIP_REPO_ROOT=/var/lib/rxpip/repo regina -a /opt/rxpip/server/rxpip_server.rexx admin adduser $u 2>&1)
    case "$o" in *"token (shown once): "*) TOK=${o##*: }; break;; esac
  done
  t=$(mktemp -d)
  for p in mathutils strutils greetutils; do
    req=; [ $p = greetutils ] && req="strutils>=1.0.0"
    tar czf $t/$p.rxpkg -C /opt/rxpip/examples/$p $p.rexx
    curl -s -X POST -H "X-Rxpip-Name: $p" -H "X-Rxpip-Version: 1.0.0" \
      ${req:+-H "X-Rxpip-Requires: $req"} \
      -H "X-Rxpip-Author: rexxpository examples" -H "X-Rxpip-Summary: $p (example)" \
      -H "X-Rxpip-Token: $TOK" --data-binary @$t/$p.rxpkg "$URL?action=PUBLISH" | grep MESSAGE
  done
  rm -rf $t
fi
curl -s "$URL?action=LIST"
