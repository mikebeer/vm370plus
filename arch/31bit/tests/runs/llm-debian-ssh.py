import sys, time, os, urllib.request, paramiko
log = sys.argv[1]; L = '/home/claude/vm370/arch/31bit/languages/llama2c'
for i in range(600):
    try:
        if 'login:' in open(log, errors='replace').read(): break
    except FileNotFoundError: pass
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
run('mkdir -p /usr/local/share/llama2c')
sf.put(L + '/run390.deb', '/usr/local/bin/llama2c'); sf.put(L + '/llama.sh', '/usr/local/bin/llama')
sf.put(L + '/tokenizer.bin', '/usr/local/share/llama2c/tokenizer.bin')
sf.put('/home/claude/scratch/rand32.bin', '/tmp/rand32.bin'); sf.put('/home/claude/scratch/rand64.bin', '/tmp/rand64.bin')
sf.close()
run('chmod 755 /usr/local/bin/llama2c /usr/local/bin/llama; ls -l /usr/local/share/llama2c')
run('llama2c /tmp/rand32.bin -z /usr/local/share/llama2c/tokenizer.bin -t 0 -n 24 -i VM')
run('llama2c /tmp/rand64.bin -z /usr/local/share/llama2c/tokenizer.bin -t 0 -n 30 -i "Once upon a time"')
run('MODEL=/tmp/rand32.bin TEMP=0.9 SEED=42 STEPS=24 llama VM')
run('llama hello; rm -f /tmp/rand32.bin /tmp/rand64.bin; sync')
try: c.exec_command('sleep 2; halt', timeout=10)
except Exception: pass
c.close()
time.sleep(60)
urllib.request.urlopen('http://127.0.0.1:8081/cgi-bin/tasks/syslog?command=%2Fmsg%20operator%20MDDEB-DONE').read()
