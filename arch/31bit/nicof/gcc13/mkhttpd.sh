#!/bin/sh
# mkhttpd.sh -- build HTTPD31 TEXT: HTTPD with NICOF's client for GCC 13
# (-m31, cmsrt).  On CMS (IPL 290): LOAD HTTPD31 (CLEAR, GENMOD HTTPD31.
set -e
H=$(cd "$(dirname "$0")" && pwd)
T=$(mktemp -d)
cp $H/../httpd/HTTPD.C $T/httpd.c          # .C would be C++ to gcc
CFLAGS_EXTRA="-D__CMSRT__ -I$H -w" sh $H/../../m5f/crt.sh ${1:-$H/HTTPD31.text} \
  $T/httpd.c $H/nicofclt.c $H/ncfsockt.c $H/ncfglue.c $H/ncfintr.s
rm -rf $T
