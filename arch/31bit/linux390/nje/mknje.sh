#!/bin/sh
# mknje.sh s390|pc [PREFIX] -- VM/370plus 9.13b: NJE-II (FUNET-NJE) for the
# Debian guest (s390, Debian 7) or for the PC (tests).
#
# The source is not part of this repository: it carries the Hebrew
# University of Jerusalem's FUNET-NJE licence.  This script fetches it from
# GitHub at a fixed commit, makes the changes below and builds it.
#   - -fcommon: the headers define MAX_LINES etc. in every object
#   - sys_nerr: glibc declares it (int); PRINT_ERRNO is strerror(errno)
# Output: nje-<target>.tgz (bin/, lib/, etc/ under PREFIX, default
# /usr/local/nje), the binaries stripped.
set -e
T=${1:-s390}; P=${2:-/usr/local/nje}
REPO=https://github.com/moshix/NJE-II.git; COMMIT=b51ed53
H=$(cd $(dirname $0) && pwd); R=$(cd $H/../../../.. && pwd)
W=$(mktemp -d); trap 'rm -rf $W' EXIT
git clone -q $REPO $W/src; (cd $W/src && git checkout -q $COMMIT)
cd $W/src
sed -i -E 's/^extern const int[[:space:]]+sys_nerr;//; s/\(errno > sys_nerr \? "\*\*\*" : strerror\(errno\)\)/strerror(errno)/' *.c *.h
echo '/* sys_nerr: glibc has it (VM/370plus) */' > sysnerr.c
D="-DUSG -DNBCONNECT -DNBSTREAM -DUSE_XMIT_QUEUE -DUNIX -DHAS_PUTENV -DUSE_SOCKOPT -DSOCKBUFSIZE=8192 -DUSE_ENUM_TYPES"
D="$D -DCONFIG_FILE='\"$P/etc/nje.cf\"' -DPID_FILE='\"$P/etc/nje.pid\"'"
D="$D -DDEF_SPOOL_DIR_RULE='\"$P/var/spool/bitnet/\"' -DDEF_POSTMAST_SPOOL_DIR='\"$P/var/spool/bitnet/POSTMAST\"' -DVMNET_IP_PORT=175"
if [ $T = s390 ]; then
  CC="$R/arch/31bit/languages/common/wcc -fcommon -O2"; ST=s390x-linux-gnu-strip
  M="RANLIB=s390x-linux-gnu-ranlib AR=s390x-linux-gnu-ar"
else
  CC="gcc -fcommon -O2"; ST=strip; M=
fi
make -s CC="$CC" CDEFS="$D" $M njed receive tell sendfile njeroutes bitsend qrdr ygone ucp transfer >/dev/null 2>$W/err || { cat $W/err; exit 1; }
O=$W/out$P; mkdir -p $O/bin $O/lib $O/etc $O/var/spool/bitnet $O/var/log
for b in njed receive tell sendfile njeroutes bitsend qrdr ygone ucp transfer; do cp $b $O/bin/; $ST $O/bin/$b; done
cp cmd-help.txt $O/lib/
cp LICENSE* COPYING* $O/lib/ 2>/dev/null || head -12 main.c > $O/lib/COPYRIGHT
(cd $W/out && tar czf $H/nje-$T.tgz .${P})
echo "mknje: $H/nje-$T.tgz"; tar tzf $H/nje-$T.tgz | grep bin/ | tr '\n' ' '; echo
