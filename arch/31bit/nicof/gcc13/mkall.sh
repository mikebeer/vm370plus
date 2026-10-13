#!/bin/sh
# mkall.sh [outdir] -- build NICOF's CMS client tools with GCC 13 (-m31, cmsrt) into
# TEXT decks (VM/370plus 9.5).  On CMS (IPL 290): LOAD name (CLEAR, GENMOD.
#   (ONLY="NHFS HACC" builds just those)
#   NHFS HACC IPLOOKUP SOCKC SOCKS TSERVER TSELECT UDPSERV NCTEST HTTPD31
set -e
H=$(cd "$(dirname "$0")" && pwd)
O=${1:-$H/out}; mkdir -p $O
B="$H/nicofclt.c $H/ncfglue.c $H/ncfintr.s"
S="$H/ncfsockt.c $B"
F="$H/nhfscomn.c $H/ncfbases.c $H/cmsfile.c $B"
cc() { n=$1; shift; [ -z "$ONLY" ] || case " $ONLY " in *" $n "*) ;; *) return 0;; esac
       CFLAGS_EXTRA="-D__CMSRT__ -D__CMS__ -I$H -include $H/cmssys.h -w" \
       sh $H/../../m5f/crt.sh $O/$n.text "$@" 2>&1 | grep -v "RWX\|GNU-stack\|deprecated" || true
       test -s $O/$n.text || { echo "mkall: $n failed"; exit 1; }; }
cc NHFS     $H/nhfs.c $H/svcnhfs.c $F
cc HACC     $H/rnhfs.c $H/svcrnhfs.c $F
cc IPLOOKUP $H/gethost.c $S
cc SOCKC    $H/sockc.c $S
cc SOCKS    $H/socks.c $S
cc TSERVER  $H/tserver.c $S
cc TSELECT  $H/tselect.c $S
cc UDPSERV  $H/udpserv.c $S
cc NCTEST   $H/nctest.c $S
T=$(mktemp -d); cp $H/../httpd/HTTPD.C $T/httpd.c
cc HTTPD31  $T/httpd.c $S
rm -rf $T $O/*.elf $O/*.map
ls -l $O
