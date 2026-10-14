#!/bin/sh
# cc.sh -- compile one C file for CMS (crt.sh's flags) with the JamVM/Classpath shims
X=s390x-linux-gnu
NEWLIB=/home/claude/nlbuild; NLSRC=/home/claude/nlsrc/newlib-4.4.0.20231231/newlib
GI=$($X-gcc -m31 -print-file-name=include)
H=/home/claude/src/java/cms
exec $X-gcc "$@" -m31 -mesa -march=z900 -msoft-float -mlong-double-64 -O2 -fno-pic -fno-pie \
 -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-stack-protector -ffunction-sections \
 -fdata-sections -nostdinc -isystem $GI -I$H/inc -I/home/claude/vm370/arch/31bit/m5f/inc \
 -I$NEWLIB/targ-include -I$NLSRC/libc/include -DHAVE_CONFIG_H -D__CMS__ -include /home/claude/src/java/cms/inc/cmscompat.h
