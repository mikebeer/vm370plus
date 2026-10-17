# interactive ssh relay: run each /home/claude/scratch/mdb/q/NNN.cmd, write NNN.out; 'HALT' ends
import sys, time, os, glob, urllib.request, urllib.parse, paramiko
log = sys.argv[1]; Q = '/home/claude/scratch/mdb/q'; os.makedirs(Q, exist_ok=True)
def term():
    try: return open(log, errors='replace').read()
    except FileNotFoundError: return ''
for i in range(1200):
    if 'login:' in term(): break
    time.sleep(3)
time.sleep(30)
for attempt in range(8):
    try:
        c = paramiko.SSHClient(); c.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        c.connect('10.1.1.2', username='root', password='vm370plus', timeout=60, banner_timeout=180, auth_timeout=180, look_for_keys=False, allow_agent=False)
        break
    except Exception as ex:
        print('attempt', attempt, repr(ex), flush=True); time.sleep(15)
else: sys.exit('no ssh')
open(Q + '/READY', 'w').write('up\n')
done = set()
while True:
    for f in sorted(glob.glob(Q + '/*.cmd')):
        if f in done: continue
        done.add(f); cmd = open(f).read().strip()
        if cmd == 'HALT':
            try: c.exec_command('sleep 2; halt', timeout=10)
            except Exception: pass
            time.sleep(120)
            urllib.request.urlopen('http://127.0.0.1:8081/cgi-bin/tasks/syslog?command=' + urllib.parse.quote('/msg operator MDDEB-DONE')).read()
            sys.exit(0)
        if cmd.startswith('PUT '):
            _, a, b = cmd.split(); s = c.open_sftp(); s.put(a, b); s.close(); out = 'put ok\n'; rc = 0
        else:
            t0 = time.time()
            try:
                i, o, e = c.exec_command(cmd, timeout=3600)
                out = o.read().decode(errors='replace') + e.read().decode(errors='replace'); rc = o.channel.recv_exit_status()
            except Exception as ex:
                out = repr(ex); rc = -1
            out += '\n[rc %s, %.0fs]\n' % (rc, time.time() - t0)
        open(f[:-4] + '.out.tmp', 'w').write(out); os.rename(f[:-4] + '.out.tmp', f[:-4] + '.out')
    time.sleep(2)
