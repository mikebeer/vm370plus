import sys, time, os, subprocess, urllib.request, urllib.parse, paramiko
log = sys.argv[1]; S = '/home/claude/vm370/arch/31bit/linux390/vmspool'
def term():
    try: return open(log, errors='replace').read()
    except FileNotFoundError: return ''
def herc(cmd):
    urllib.request.urlopen('http://127.0.0.1:8081/cgi-bin/tasks/syslog?command=' + urllib.parse.quote(cmd)).read()
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
sf = c.open_sftp()
run('rm -rf /root/vmspool-new; mkdir -p /root/vmspool-new/handlers /root/vmspool-new/linux')
for root, dirs, files in os.walk(S):
    rel = os.path.relpath(root, S)
    if rel not in ('.', 'handlers', 'linux'): continue
    for f in files:
        if f.endswith('.pyc'): continue
        sf.put(os.path.join(root, f), '/root/vmspool-new/' + ('' if rel == '.' else rel + '/') + f)
sf.close()
run('cd /root/vmspool-new && chmod +x vmspoold handlers/* linux/* && sh install.sh 2>&1 | tail -1; cp -p /etc/vmspool.d/mail.conf /root/mail.conf.save; cp -p /etc/vmspool.d/ask.conf /root/ask.conf.save; ls -l /etc/vmspool.d/setup')
open("/tmp/claude-0/wiz/a2.json", "w").write('{"smarthost": "mail.example.com:587", "smartuser": "bob", "smartpass": "secret", "ai_model": "qwen2.5", "ai_key": "sk-ant-' + "x" * 100 + '", "nje_peers": "PCNJE:10.1.1.1 ZVM1:192.0.2.9:175"}')
r = subprocess.run([sys.executable, '/home/claude/vm370/arch/31bit/kit/vm370plus-setup.py', '--apply', '/tmp/claude-0/wiz/inst', '--answers', '/tmp/claude-0/wiz/a2.json'], capture_output=True, text=True)
print('PC$ wizard --apply:', r.returncode, r.stdout, r.stderr, flush=True)
time.sleep(60)
herc('/q rdr operator all')
run('grep -v "^#" /etc/vmspool.d/mail.conf; grep -v "^#" /etc/vmspool.d/ask.conf; grep -A1 "^LINE\\|^NAME\\|DEFAULT" /usr/local/nje/etc/nje.cf; grep -h setup /var/log/syslog /var/log/daemon.log 2>/dev/null | tail -3')
# back as it was
run('cp -p /root/mail.conf.save /etc/vmspool.d/mail.conf; cp -p /root/ask.conf.save /etc/vmspool.d/ask.conf; /usr/local/nje/bin/njeconf /usr/local/nje VM370CE 10.1.1.2 PCNJE:10.1.1.1; /etc/init.d/nje restart; /etc/init.d/vmmaild restart; rm -rf /root/vmspool-new /root/*.save; rm -f /usr/local/nje/var/log/*; sync')
herc('/msg operator WIZ-END')
try: c.exec_command('sleep 2; halt', timeout=10)
except Exception: pass
c.close()
time.sleep(90)
herc('/msg operator MDDEB-DONE')
