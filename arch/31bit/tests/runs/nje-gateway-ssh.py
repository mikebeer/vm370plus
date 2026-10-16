import sys, time, os, subprocess, urllib.request, urllib.parse, paramiko
log = sys.argv[1]; N = '/home/claude/vm370/arch/31bit/linux390/nje'; S = '/home/claude/vm370/arch/31bit/linux390/vmspool'; PC = '/home/claude/nje-pc'
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
def run(cmd, t=1800):
    i, o, e = c.exec_command(cmd, timeout=t)
    out = o.read().decode(errors='replace') + e.read().decode(errors='replace')
    print('$', cmd); print(out, flush=True); return out
sf = c.open_sftp()
run('rm -rf /root/vmspool-new /root/nje-new; mkdir -p /root/vmspool-new/handlers /root/vmspool-new/linux /root/nje-new')
for root, dirs, files in os.walk(S):
    rel = os.path.relpath(root, S)
    if rel not in ('.', 'handlers', 'linux'): continue
    for f in files:
        if f.endswith('.pyc'): continue
        sf.put(os.path.join(root, f), '/root/vmspool-new/' + ('' if rel == '.' else rel + '/') + f)
for f in ('nje-s390.tgz', 'njeconf.sh', 'njefile', 'njemsg', 'njecards', 'nje.init', 'install.sh'):
    sf.put(N + '/' + f, '/root/nje-new/' + f)
sf.close()
run('cd /root/vmspool-new && chmod +x vmspoold handlers/* linux/* && sh install.sh 2>&1 | tail -2; ls /etc/vmspool.d/')
run('cd /root/nje-new && FORCE=1 sh install.sh 2>&1 | tail -3; sleep 5; pgrep -x njed; tail -n 5 /usr/local/nje/var/log/bitnet.log')
for i in range(900):
    if 'NJT-READY' in term(): break
    time.sleep(3)
time.sleep(90)
run('tail -n 30 /usr/local/nje/var/log/bitnet.log; cat /usr/local/nje/var/log/njefile.log /usr/local/nje/var/log/njemsg.log 2>/dev/null; grep -h "nje" /var/log/syslog /var/log/daemon.log 2>/dev/null | tail -5')
pc('ls -l %s/var/in; tail -n 5 %s/var/in/messages; cat %s/var/in/PCUSER.CMS.EXEC.* 2>/dev/null | head -5' % (PC, PC, PC))
pc('printf "line one from the PC\\nline two\\n" > /tmp/njetest.txt; %s/bin/sendfile cmsuser@VM370CE -u PCUSER -fn NJETEST DATA /tmp/njetest.txt; %s/bin/tell -u PCUSER cmsuser@VM370CE "Hello CMSUSER from the PC"' % (PC, PC))
time.sleep(60)
run('tail -n 12 /usr/local/nje/var/log/bitnet.log; cat /usr/local/nje/var/log/njefile.log /usr/local/nje/var/log/njemsg.log; ls -l /var/spool/vmspool/outq /var/spool/vmspool/nje')
sf = c.open_sftp()
try: sf.get('/var/spool/vmspool/nje/last.spool', '/home/claude/scratch/nje/last.spool')
except Exception as e: print('no last.spool', e)
sf.close()
herc('/msg cmsuser NJT-IN')
for i in range(600):
    if 'NJT-END' in term(): break
    time.sleep(3)
run('rm -rf /var/spool/vmspool/outq/* /var/spool/vmspool/nje/* /usr/local/nje/var/log/* /root/vmspool-new /root/nje-new; ls /usr/local/nje/etc; cat /usr/local/nje/etc/nje.cf | head -3; sync')
try: c.exec_command('sleep 2; halt', timeout=10)
except Exception: pass
c.close()
time.sleep(60)
herc('/msg operator MDDEB-DONE')
