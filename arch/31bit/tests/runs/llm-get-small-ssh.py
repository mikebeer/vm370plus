import sys, time, os, threading, functools, urllib.request, paramiko
from http.server import HTTPServer, SimpleHTTPRequestHandler
log = sys.argv[1]; src = '/home/claude/vm370/arch/31bit/linux390/vmspool'; L = '/home/claude/vm370/arch/31bit/languages/llama2c'
def term():
    try: return open(log, errors='replace').read()
    except FileNotFoundError: return ''
def web():
    H = functools.partial(SimpleHTTPRequestHandler, directory='/home/claude/scratch')
    for i in range(400):
        try: s = HTTPServer(('10.1.1.1', 8000), H); break
        except OSError: time.sleep(3)
    print('web server up on 10.1.1.1:8000', flush=True); s.serve_forever()
threading.Thread(target=web, daemon=True).start()
for i in range(900):
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
def run(cmd, t=3600):
    i, o, e = c.exec_command(cmd, timeout=t)
    out = o.read().decode(errors='replace') + e.read().decode(errors='replace')
    print('$', cmd); print(out, flush=True); return out
sf = c.open_sftp()
run('rm -rf /root/vmspool-new; mkdir -p /root/vmspool-new/handlers /root/vmspool-new/linux')
for root, dirs, files in os.walk(src):
    rel = os.path.relpath(root, src)
    if rel not in ('.', 'handlers', 'linux'): continue
    for f in files:
        sf.put(os.path.join(root, f), '/root/vmspool-new/' + ('' if rel == '.' else rel + '/') + f)
sf.put(L + '/llama.sh', '/usr/local/bin/llama'); sf.put(L + '/llama.conf', '/etc/llama.conf')
sf.close()
run('cd /root/vmspool-new && chmod +x vmspoold handlers/* linux/* && sh install.sh 2>&1 | tail -2; chmod 755 /usr/local/bin/llama; which wget curl')
run("rm -rf /var/spool/vmspool/outq/* /usr/local/share/llama2c/stories15M.bin; sed -i 's|^URL=.*|URL=http://10.1.1.1:8000/rand32.bin|; s|^MD5=.*|MD5=|' /etc/llama.conf; cat /etc/llama.conf")
print('PHASE1-DONE', flush=True)
for i in range(1800):
    if 'LLGET-END' in term(): break
    time.sleep(5)
time.sleep(10)
run('grep -h vmspoold /var/log/syslog /var/log/daemon.log 2>/dev/null | tail -5; ls -l /var/spool/vmspool/outq')
sf = c.open_sftp(); sf.put(L + '/llama.conf', '/etc/llama.conf'); sf.close()
run('rm -rf /usr/local/share/llama2c/stories15M.bin /var/spool/vmspool/outq/* /var/spool/vmspool/out/*; cat /etc/llama.conf; ls -la /var/spool/vmspool/outq /usr/local/share/llama2c; rm -rf /root/vmspool-new; sync')
try: c.exec_command('sleep 2; halt', timeout=10)
except Exception: pass
c.close()
time.sleep(60)
urllib.request.urlopen('http://127.0.0.1:8081/cgi-bin/tasks/syslog?command=%2Fmsg%20operator%20MDDEB-DONE').read()
