#!/bin/sh
# njeconf.sh PREFIX NODE IPADDR PEER:HOST[:PORT] ... -- VM/370plus 9.13b:
# the configuration of an NJE-II node (nje.cf, the file and message exits,
# the routing table).  Each PEER is a TCPNJE link (line); the first is also
# the default route.  Files and messages for local users go to
# PREFIX/lib/njefile and PREFIX/lib/njemsg (installed beside this).
set -e
P=$1 N=$(echo $2 | tr a-z A-Z) IP=$3; shift 3
mkdir -p $P/etc $P/lib $P/var/spool/bitnet/POSTMAST $P/var/log
F=$(echo ${1%%:*} | tr a-z A-Z)
{
echo "* nje.cf -- written by njeconf.sh (VM/370plus 9.13b)"
echo "NAME		$N"
echo "IPADDRESS	$IP"
echo "QUEUE		$P/var/spool/bitnet"
echo "CMDMAILBOX	U 127.0.0.1 175"
echo "LOG		$P/var/log/bitnet.log"
echo "RSCSACCT	$P/var/log/bitnet.acct"
echo "TABLE		$P/lib/nje.route"
echo "INFORM		OPERATOR@$N"
echo "FILEEXITS	$P/lib/file-exit.cf"
echo "MSGEXITS	$P/lib/msg-exit.cf"
echo "DEFFORM		STANDARD"
echo "LLEVEL		2"
[ -n "$F" ] && echo "DEFAULT-ROUTE	$F"
i=0
for p in "$@"; do
  L=$(echo ${p%%:*} | tr a-z A-Z); r=${p#*:}; H=${r%%:*}; PO=175
  [ "$r" != "$H" ] && PO=${r#*:}
  echo
  echo "LINE $i $L"
  echo " TYPE		UNIX_TCP"
  echo " BUFSIZE	1024"
  echo " TIMEOUT	3"
  echo " IPPORT		$PO"
  echo " TCPNAME	$H"
  echo " TCP-SIZE	8192"
  echo " MAX-STREAMS	7"
  i=$((i + 1))
done
} > $P/etc/nje.cf
{
echo "Spool-Dir:	$P/var/spool/bitnet/"
echo "Postmast-Dir:	$P/var/spool/bitnet/POSTMAST"
echo "Exit-Table:"
echo "* $N * * * * * * * default RUN $P/lib/njefile \$SPOOL \$TOUSER \$FRUSER \$FRNODE \$FNAME \$FTYPE \$CLASS"
echo "* * * * * * * * * default KEEP"
} > $P/lib/file-exit.cf
{
echo "CmdHelpFile: $P/lib/cmd-help.txt"
for c in '"H*ELP"' '"M * * * **"' '"MSG * * * **"' '"Q*UERY SYS*TEM"' '"Q*UERY *"' '"CPQ*UERY N*AMES"' '"CPQ*UERY U*SER *"' '"CPQ*UERY T*IME"'; do
  case $c in '"H*ELP"') a=HELP;; *) a=HARDCODED;; esac
  echo ".        $N *        *        C $c BUILTIN $a"
done
echo ".        $N *        *        C \"**\" BUILTIN ERROR"
echo "VMNET    *  *        *        M  DISCARD"
echo "*        $N *        *        M  RUN $P/lib/njemsg \$FRUSER \$FRNODE \$TOUSER \$TEXT"
} > $P/lib/msg-exit.cf
{
echo "* routes -- written by njeroutes.sh"
echo "ROUTE $N LOCAL ASCII"
for p in "$@"; do L=$(echo ${p%%:*} | tr a-z A-Z); echo "ROUTE $L $L EBCDIC"; done
} > $P/lib/njeroutes.header
: > $P/lib/njeroutes.netinit
(cd $P/lib && $P/bin/njeroutes njeroutes.header njeroutes.netinit nje.route >/dev/null)
echo "njeconf: node $N ($IP), lines: $*"
