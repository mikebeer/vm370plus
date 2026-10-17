# kt40 helper (kit b26, fresh CE): NJE between CMSUSER and an NJE-II on the
# PC (node PCNJE, /home/claude/nje-pc, peer VM370CE:10.1.1.2); nothing is
# installed -- the kit's Debian disk and pack carry it all.
import sys, time, subprocess, urllib.request, urllib.parse, paramiko
log = sys.argv[1]; PC = '/home/claude/nje-pc'
def term():
    try: return open(log, errors='replace').read()
    except FileNotFoundError: return ''
def herc(cmd):
    urllib.request.urlopen('http://127.0.0.1:8081/cgi-bin/tasks/syslog?command=' + urllib.parse.quote(cmd)).read()
def pc(cmd):
    out = subprocess.run(cmd, shell=True, capture_output=True, text=True); print('PC$', cmd); print(out.stdout + out.stderr, flush=True)
for i in range(1200):
    if 'login:' in term(): break
    time.sleep(3)
time.sleep(20)
for attempt in range(8):
    try:
        c = paramiko.SSHClient(); c.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        c.connect('10.1.1.2', username='root', password='vm370plus', timeout=60, banner_timeout=180, auth_timeout=180, look_for_keys=False, allow_agent=False)
        break
    except Exception as ex:
        print('attempt', attempt, repr(ex), flush=True); time.sleep(15)
else: sys.exit('no ssh')
def run(cmd, t=600):
    i, o, e = c.exec_command(cmd, timeout=t)
    out = o.read().decode(errors='replace') + e.read().decode(errors='replace')
    print('$', cmd); print(out, flush=True); return out
run('/etc/init.d/nje status; head -3 /usr/local/nje/etc/nje.cf; ls /etc/vmspool.d | tr "\\n" " "')
for i in range(900):
    if 'NJT-READY' in term(): break
    time.sleep(3)
time.sleep(90)
pc('ls -l %s/var/in; tail -n 3 %s/var/in/messages; head -3 %s/var/in/PCUSER.CMS.EXEC.*' % (PC, PC, PC))
pc('printf "line one from the PC\\nline two\\n" > /tmp/njetest.txt; %s/bin/sendfile cmsuser@VM370CE -u PCUSER -fn NJETEST DATA /tmp/njetest.txt; %s/bin/tell -u PCUSER cmsuser@VM370CE "Hello CMSUSER from the PC"' % (PC, PC))
time.sleep(60)
run('cat /usr/local/nje/var/log/njefile.log /usr/local/nje/var/log/njemsg.log')
herc('/msg cmsuser NJT-IN')
for i in range(600):
    if 'NJT-END' in term(): break
    time.sleep(3)
try: c.exec_command('sleep 2; halt', timeout=10)
except Exception: pass
c.close()
time.sleep(60)
herc('/msg operator MDDEB-DONE')
