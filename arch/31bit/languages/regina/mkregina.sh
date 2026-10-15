#!/bin/bash
# mkregina.sh SRC OUT -- 9.37: Regina REXX 3.9.5 for CMS (M5f runtime),
# cross-built with m5f/cmscc.  SRC: a Regina source tree (github
# 0branch/regina, the 3.9.5 sources).  OUT/regina.text: a TEXT deck,
# LOAD/GENMOD on CMS as REGINA MODULE.
#   - the generic OS layer (os_other.c; os_unx.c wants fork/exec, uname):
#     commands go to cmsrt's system(), i.e. SVC 202 (ADDRESS CMS/COMMAND)
#   - no external queues (rxstack needs sockets): -DNO_EXTERNAL_QUEUES
#   - 32-bit (configure guesses 64 from the PC)
set -e
H=$(cd "$(dirname "$0")" && pwd); M=$(cd "$H/../../m5f" && pwd)
S=$(cd "$1" && pwd); O=$(mkdir -p "$2" && cd "$2" && pwd)
cd $S
git checkout -- os_other.c misc.c stack.c 2>/dev/null || true
patch -p1 < $H/regina-cms.patch
python3 $H/stack-cms.py stack.c
cp $H/cmscompat.c $H/cmsexeccomm.c .
CC=$M/cmscc ./configure --host=s390-linux-gnu --build=x86_64-linux-gnu \
  --disable-gci --disable-posix-threads >/dev/null
sed -i 's/os_unx\.\$(OBJ)/os_other.$(OBJ)/; s/-DREGINA_BITS=64/-DREGINA_BITS=32/' Makefile
sed -i '/^os_other\.\$(OBJ) :/,+1s/os_unx\.c/os_other.c/' Makefile
sed -i 's/^\(OFILES = funcs.$(OBJ)\)/\1 cmscompat.$(OBJ) cmsexeccomm.$(OBJ)/' Makefile
printf 'cmsexeccomm.$(OBJ) : cmsexeccomm.c rexx.h\n\t$(CC) $(COPT) $(CC2O) -c cmsexeccomm.c\n' >> Makefile
rm -f *.o rexx
$M/cmscc -DNDEBUG -O2 -I. -c cmscompat.c -o cmscompat.o   # (no rule in Regina's Makefile)
make rexx CFLAGS="-DNDEBUG -O2 -DNO_EXTERNAL_QUEUES" >/dev/null
${E2C:-/home/claude/m5f/elf_to_cms} rexx $O/regina.text
# 9.39: the same as an XIMAGE (high storage), for XL and the resident XLRES
rm -f *.o rexx
CMSCC_XL=1 $M/cmscc -DNDEBUG -O2 -I. -c cmscompat.c -o cmscompat.o
CMSCC_XL=1 CMSCC_XL=1 make rexx CFLAGS="-DNDEBUG -O2 -DNO_EXTERNAL_QUEUES" >/dev/null
python3 $M/mkximage.py rexx $O/regina.ximage
