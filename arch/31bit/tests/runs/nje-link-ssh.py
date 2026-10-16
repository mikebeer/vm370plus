import sys, time, os, subprocess, urllib.request, urllib.parse, paramiko
log = sys.argv[1]; N = '/home/claude/vm370/arch/31bit/linux390/nje'; PC = '/home/claude/nje-pc'
def term():
    try: return open(log, errors='replace').read()
    except FileNotFoundError: return ''
def herc(cmd):
    urllib.request.urlopen('http://127.0.0.1:8081/cgi-bin/tasks/syslog?command=' + urllib.parse.quote(cmd)).read()
def pc(cmd):
    out = subprocess.run(cmd, shell=True, capture_output=True, text=True); print('PC$', cmd); print(out.stdout + out.stderr, flush=True)
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
def run(cmd, t=1800):
    i, o, e = c.exec_command(cmd, timeout=t)
    out = o.read().decode(errors='replace') + e.read().decode(errors='replace')
    print('$', cmd); print(out, flush=True); return out
sf = c.open_sftp()
sf.put(N + '/nje-s390.tgz', '/root/nje-s390.tgz')
for f in ('njeconf.sh', 'njefile', 'njemsg'): sf.put(N + '/' + f, '/root/' + f)
sf.close()
run('tar xzf /root/nje-s390.tgz -C / && cp /root/njefile /root/njemsg /usr/local/nje/lib/ && chmod +x /usr/local/nje/lib/nje* && sh /root/njeconf.sh /usr/local/nje VM370P 10.1.1.2 PCNJE:10.1.1.1 && cd /usr/local/nje && (setsid ./bin/njed >/dev/null 2>&1 &) ; sleep 5; ps ax | grep [n]jed; tail -n 15 /usr/local/nje/var/log/bitnet.log')
# wait until CMSUSER is logged on (the driver says so)
for i in range(600):
    if 'NJT-READY' in term(): break
    time.sleep(3)
time.sleep(30)
run('tail -n 20 /usr/local/nje/var/log/bitnet.log')
pc('tail -n 20 %s/var/log/bitnet.log' % PC)
# PC -> VM: a message and a file for CMSUSER
pc('%s/bin/tell -u PCUSER cmsuser@VM370P "Hello from the PC over NJE"' % PC)
pc('printf "line one from the PC\\nline two\\n" > /tmp/njetest.txt; %s/bin/sendfile cmsuser@VM370P -u PCUSER -fn NJETEST DATA /tmp/njetest.txt' % PC)
time.sleep(60)
run('tail -n 30 /usr/local/nje/var/log/bitnet.log; cat /usr/local/nje/var/log/njefile.log /usr/local/nje/var/log/njemsg.log; ls -l /var/spool/vmspool/outq')
# VM -> PC
run('/usr/local/nje/bin/tell -u MAINT pcuser@PCNJE "Hello from VM/370plus"; printf "a file from Debian on VM/370\\n" > /tmp/back.txt; /usr/local/nje/bin/sendfile pcuser@PCNJE -u MAINT -fn BACK TEXT /tmp/back.txt; sleep 40; tail -n 12 /usr/local/nje/var/log/bitnet.log')
pc('ls -l %s/var/in; cat %s/var/in/*; tail -n 20 %s/var/log/bitnet.log' % (PC, PC, PC))
herc('/msg cmsuser NJT-IN')
for i in range(600):
    if 'NJT-END' in term(): break
    time.sleep(3)
run('pkill njed; sleep 2; rm -rf /usr/local/nje /root/nje* /var/spool/vmspool/outq/* /tmp/back.txt; sync')
try: c.exec_command('sleep 2; halt', timeout=10)
except Exception: pass
c.close()
time.sleep(60)
herc('/msg operator MDDEB-DONE')
