#!/bin/bash
# M8: current cREXX (adesutherland/crexx, ports/single-threaded) cross-built
# for CMS on VM/370plus with the M5f toolchain (GCC 13 -m31, newlib, cmsrt).
#   build.sh vm|rxc|rxas   -> rxbvm.text / rxc.text / rxas.text here
set -e
export E2C=${E2C:-/home/claude/m5f/elf_to_cms8}   # RXC is 2.8 MB: the 8 MB image limit
cd "$(dirname "$0")"
. ./units.sh
case "$1" in
vm) ../m5f/crt.sh rxbvm.text $C/interpreter/rxvmmain.c $VMF ;;
rxc) LIBSRC="$VMF $AS" CFLAGS_EXTRA="$CFLAGS_EXTRA -I$H/compiler -I$H/assembler" ../m5f/crt.sh rxc.text $C/compiler/rxc_main.c $CP $C/interpreter/rxvml.c ;;
rxas) CFLAGS_EXTRA="$CFLAGS_EXTRA -I$H/assembler" ../m5f/crt.sh rxas.text $C/assembler/rxasmain.c $AS $VMF ;;
*) echo "usage: build.sh vm"; exit 2 ;;
esac
