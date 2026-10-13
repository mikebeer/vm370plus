#!/bin/sh
# nicof.sh start|stop|status -- the NICOF outside proxies on the PC (9.13d)
# Run it from the VM/370plus directory while Hercules is up: the proxies
# connect to Hercules' 3270 port (localhost:3270) and DIAL the inside
# proxies NICOFPXY (host files, NHFS/HACC) and TCPIPPXY (sockets, CMSFTPD)
# on the 3270 lines of the groups of the same name (03CD, 03CE).
cd "$(dirname "$0")"
P=nicof.pid
case "$1" in
start)
  command -v java >/dev/null || { echo "nicof: java is not on the PATH"; exit 1; }
  [ -s $P ] && kill -0 "$(cat $P)" 2>/dev/null && { echo "nicof: already running"; exit 0; }
  java -cp .:nicof.jar dev.hawala.vm370.commproxy.CommProxy \
       nicofpxy.properties tcpip.properties > nicof.log 2>&1 &
  echo $! > $P; echo "nicof: started (pid $(cat $P), log nicof.log)" ;;
stop)
  [ -s $P ] && kill "$(cat $P)" 2>/dev/null; rm -f $P; echo "nicof: stopped" ;;
status)
  [ -s $P ] && kill -0 "$(cat $P)" 2>/dev/null && echo "nicof: running" || echo "nicof: not running" ;;
*) echo "usage: $0 start|stop|status"; exit 2 ;;
esac
