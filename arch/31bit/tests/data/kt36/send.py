#!/usr/bin/env python3
# kt36 helper: once CMSUSER is at Ready (IPL 290), spool two files from the
# PC with vmspool.py: a short EXEC (text, 010C) and the Markdown sample
# (lines over 80: a stream, 020C)
import subprocess, sys, time
log = sys.argv[-1]; T = '/home/claude/vm370/arch/31bit/tools/vmspool.py'
for i in range(400):
    try:
        if 'Ready' in open(log, errors='replace').read(): break
    except FileNotFoundError: pass
    time.sleep(3)
time.sleep(10)
for a in (['/home/claude/vm370/arch/31bit/tests/data/kt36/pchello.exec', 'cmsuser'],
          ['/home/claude/vm370/arch/31bit/languages/markdown/SAMPLE.md', 'cmsuser']):
    print(subprocess.run(['python3', T] + a, capture_output=True, text=True).stdout, flush=True)
