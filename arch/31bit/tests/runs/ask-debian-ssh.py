import sys, time, os, threading, json, urllib.request, paramiko
from http.server import BaseHTTPRequestHandler, HTTPServer
log = sys.argv[1]; src = '/home/claude/vm370/arch/31bit/linux390/vmspool'
def term():
    try: return open(log, errors='replace').read()
    except FileNotFoundError: return ''
sys.path.insert(0, '/home/claude/vm370/arch/31bit/tools')
import importlib.util
spec = importlib.util.spec_from_file_location('fakellm_h', '/home/claude/vm370/arch/31bit/tools/fakellm.py')
def llm():
    src_ = open('/home/claude/vm370/arch/31bit/tools/fakellm.py').read().split('HTTPServer((')[0]
    g = {}; exec(src_, g)
    for i in range(300):
        try: s = HTTPServer(('10.1.1.1', 8080), g['H']); break
        except OSError: time.sleep(3)
    print('fake LLM up on 10.1.1.1:8080', flush=True); s.serve_forever()
threading.Thread(target=llm, daemon=True).start()
for i in range(600):
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
    print('$', cmd); print(out, end='', flush=True); return out
sf = c.open_sftp()
run('rm -rf /root/vmspool-new; mkdir -p /root/vmspool-new/handlers /root/vmspool-new/linux')
for root, dirs, files in os.walk(src):
    rel = os.path.relpath(root, src)
    if rel not in ('.', 'handlers', 'linux'): continue
    for f in files:
        sf.put(os.path.join(root, f), '/root/vmspool-new/' + ('' if rel == '.' else rel + '/') + f)
sf.close()
run('cd /root/vmspool-new && chmod +x vmspoold handlers/* linux/* && sh install.sh 2>&1 | tail -3; grep -v "^#" /etc/vmspool.d/ask.conf')
run('ask What is VM/370?; ask new')
print('PHASE1-DONE', flush=True)
for i in range(400):
    if 'ASKTEST-END' in term(): break
    time.sleep(5)
time.sleep(10)
run('ls /var/spool/vmspool/ask; rm -rf /var/spool/vmspool/ask/* /root/vmspool-new; sync')
try: c.exec_command('sleep 2; halt', timeout=10)
except Exception: pass
c.close()
time.sleep(60)
urllib.request.urlopen('http://127.0.0.1:8081/cgi-bin/tasks/syslog?command=%2Fmsg%20operator%20MDDEB-DONE').read()
time.sleep(5)
