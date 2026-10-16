import sys, time, os, paramiko
log = sys.argv[1]; src = '/home/claude/vm370/arch/31bit/linux390/vmspool'
for i in range(600):
    try:
        if 'login:' in open(log, errors='replace').read(): break
    except FileNotFoundError: pass
    time.sleep(3)
time.sleep(20)
for attempt in range(8):
    try:
        c = paramiko.SSHClient(); c.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        c.connect('10.1.1.2', username='root', password='vm370plus', timeout=60, banner_timeout=180,
                  auth_timeout=180, look_for_keys=False, allow_agent=False)
        break
    except Exception as ex:
        print('attempt', attempt, repr(ex), flush=True); time.sleep(15)
else:
    sys.exit('no ssh')
def run(cmd, t=600):
    i, o, e = c.exec_command(cmd, timeout=t)
    out = o.read().decode(errors='replace') + e.read().decode(errors='replace')
    print('$', cmd); print(out, end='', flush=True)
    return out
sf = c.open_sftp()
run('rm -rf /root/vmspool-new; mkdir -p /root/vmspool-new/handlers /root/vmspool-new/linux')
for root, dirs, files in os.walk(src):
    rel = os.path.relpath(root, src)
    if rel.startswith('cms'): continue
    for f in files:
        d = '/root/vmspool-new/' + ('' if rel == '.' else rel + '/') + f
        if rel not in ('.', 'handlers', 'linux'): continue
        sf.put(os.path.join(root, f), d)
sf.put('/home/claude/vm370/arch/31bit/languages/markdown/SAMPLE.md', '/root/SAMPLE.md')
sf.close()
run('cd /root/vmspool-new && chmod +x vmspoold handlers/* linux/* && sh install.sh; echo rc=$?')
run('ls -l /usr/local/bin/md2ms /usr/local/bin/mdprint /etc/vmspool.d/markdown; python3 -V')
run('md2ms --text --width 60 /root/SAMPLE.md')
run('mdprint /root/SAMPLE.md -w 50 | head -20')
run('mdprint /root/SAMPLE.md -ps /tmp/s.ps; head -c 30 /tmp/s.ps; echo; grep -c "^%%Page:" /tmp/s.ps')
run('md2ms /root/SAMPLE.md | head -20')
run('VMSPOOL_FROM=CMSUSER /etc/vmspool.d/markdown PS sample < /root/SAMPLE.md | head -3')
run('/etc/vmspool.d/markdown TEXT 40 < /root/SAMPLE.md | head -12')
run('/etc/init.d/vmspoold status; sync')
try: c.exec_command('sleep 2; halt', timeout=10)
except Exception: pass
c.close()
import urllib.request; time.sleep(60)
urllib.request.urlopen('http://127.0.0.1:8081/cgi-bin/tasks/syslog?command=%2Fmsg%20operator%20MDDEB-DONE').read()
