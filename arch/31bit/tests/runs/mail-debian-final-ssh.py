import sys, time, os, urllib.request, paramiko
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
sf.put('/home/claude/scratch/mail/vm370plus-lan', '/usr/local/sbin/vm370plus-lan')
sf.close()
run('cd /root/vmspool-new && chmod +x vmspoold handlers/* linux/* && sh install.sh 2>&1 | tail -3')
run("sed -i 's/^SMARTHOST=.*/SMARTHOST=/' /etc/vmspool.d/mail.conf; grep -v '^#' /etc/vmspool.d/mail.conf; /etc/init.d/vmmaild restart")
run('chmod 755 /usr/local/sbin/vm370plus-lan; /usr/local/sbin/vm370plus-lan; grep vm370plus-lan /etc/hosts; cat /etc/resolv.conf; getent hosts maint cmsuser.vm370plus.lan border')
run('python3 -c "import sys; sys.path.insert(0,\'/usr/local/lib/vmspool\'); import vmmail; print(\'myid\', vmmail.myid())"')
run("printf 'To: maint\\nSubject: to myself\\n\\nkept locally\\n' | VMSPOOL_FROM=MAINT /etc/vmspool.d/mail SEND; ls /var/spool/vmspool/mail/maint /var/spool/vmspool/outq")
run('rm -rf /var/spool/vmspool/mail/* /var/spool/vmspool/outq/* /root/vmspool-new; sync')
try: c.exec_command('sleep 2; halt', timeout=10)
except Exception: pass
c.close()
time.sleep(60)
urllib.request.urlopen('http://127.0.0.1:8081/cgi-bin/tasks/syslog?command=%2Fmsg%20operator%20MDDEB-DONE').read()
