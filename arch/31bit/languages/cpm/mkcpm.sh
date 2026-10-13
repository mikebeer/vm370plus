#!/bin/sh
# mkcpm.sh [OUT] -- CP/M 2.2 for VM/370plus (9.33): RunCPM (MIT, Mockba the
# Borg) with runcpm-vm370plus.patch, built twice:
#   OUT/RunCPM       Debian guest (31-bit, wheezy glibc): runcpm, the
#                    vmspoold service cpm (CPM ... (LINUX on CMS)
#   OUT/runcpm.text  CMS (GCC 13, cmsrt): RUNCPM MODULE on the language disk
set -e
H=$(cd "$(dirname "$0")" && pwd); O=$(mkdir -p "${1:-$H/out}" && cd "${1:-$H/out}" && pwd)
W=$(mktemp -d); cd $W
git clone -q https://github.com/MockbaTheBorg/RunCPM.git && cd RunCPM && git checkout -q 79396be665d6ec263a2728b20b177f4149647ed8
git apply $H/runcpm-vm370plus.patch
cd RunCPM
$H/../common/wcc -O2 -w -DCCP_DR -DSTREAMIO -o $O/RunCPM main.c
$H/../../m5f/cmscc -O2 -w -DCCP_INTERNAL -o $O/runcpm.elf main.c
${E2C:-/home/claude/m5f/elf_to_cms} $O/runcpm.elf $O/runcpm.text
rm -rf $W
