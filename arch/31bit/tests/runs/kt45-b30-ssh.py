# kt45 helper (kit b30, fresh CE): MariaDB, MCproject (8080) and MChelpdesk
# (8081) come up at boot from the kit's Debian disk; called from the PC.
import sys, time, subprocess, urllib.request, urllib.parse, paramiko
log = sys.argv[1]
def term():
    try: return open(log, errors='replace').read()
    except FileNotFoundError: return ''
def opmsg(t): urllib.request.urlopen('http://127.0.0.1:8081/cgi-bin/tasks/syslog?command=' + urllib.parse.quote('/msg operator ' + t)).read()
def pc(cmd):
    out = subprocess.run(cmd, shell=True, capture_output=True, text=True); print('PC$', cmd); print(out.stdout + out.stderr, flush=True); return out.stdout
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
def run(cmd, t=900):
    i, o, e = c.exec_command(cmd, timeout=t)
    out = o.read().decode(errors='replace') + e.read().decode(errors='replace')
    print('$', cmd); print(out, flush=True); return out
for i in range(60):
    if 'running' in run('/etc/init.d/mariadb status'): break
    time.sleep(20)
run('netstat -ltn | grep -E ":(3306|8080|8081) "; free')
pc('curl -sS -m 300 http://10.1.1.2:8080/api/v1/health')
pc('curl -sS -m 300 -o /dev/null -w "%{http_code}\\n" http://10.1.1.2:8081/login')
opmsg('MDB-READY')
for i in range(1200):
    if 'MDB-END' in term(): break
    time.sleep(3)
try: c.exec_command('sleep 2; halt', timeout=10)
except Exception: pass
time.sleep(120)
opmsg('MDDEB-DONE')
