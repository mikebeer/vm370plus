#!/bin/sh
# mkterse.sh OUT -- terse and unterse (9.22) for CMS (OUT/trs.text, untrs.text:
# LOAD, GENMOD TRS / UNTRS; TERSE and UNTERSE EXEC) and Debian (OUT/terse,
# OUT/unterse: 31-bit wheezy, via wcc)
set -e
H=$(cd "$(dirname "$0")" && pwd); M=$H/../../m5f; O=$(mkdir -p "$1" && cd "$1" && pwd)
"$M/cmscc" -O2 -o $O/untrs.elf $H/unterse.c
${E2C:-/home/claude/m5f/elf_to_cms} $O/untrs.elf $O/untrs.text
"$H/../common/wcc" -std=c89 -O2 -o $O/unterse $H/unterse.c
"$M/cmscc" -O2 -o $O/trs.elf $H/terse.c
${E2C:-/home/claude/m5f/elf_to_cms} $O/trs.elf $O/trs.text
"$H/../common/wcc" -std=c89 -O2 -o $O/terse $H/terse.c
ls -l $O/untrs.text $O/unterse $O/trs.text $O/terse
