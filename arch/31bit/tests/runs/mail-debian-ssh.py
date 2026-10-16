import sys, time, os, threading, smtpd, asyncore, smtplib, urllib.request, paramiko
from email.mime.text import MIMEText
log = sys.argv[1]; src = '/home/claude/vm370/arch/31bit/linux390/vmspool'
def term(): 
    try: return open(log, errors='replace').read()
    except FileNotFoundError: return ''
# the smarthost on the PC: 10.1.1.1:2526 (exists once Hercules made the tun)
class Sink(smtpd.SMTPServer):
    def process_message(self, peer, mf, rc, data, **k):
        print('SINK got from %s for %s:\n%s' % (mf, rc, data if isinstance(data, str) else data.decode()), flush=True)
def sink():
    for i in range(300):
        try: Sink(('10.1.1.1', 2526), None); break
        except OSError: time.sleep(3)
    asyncore.loop()
threading.Thread(target=sink, daemon=True).start()
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
run('python3 -c "import smtpd, smtplib, asyncore, email.mime.text; print(\'python mail modules ok\')"')
run('cd /root/vmspool-new && chmod +x vmspoold handlers/* linux/* && sh install.sh 2>&1 | tail -5; echo rc=$?')
run("sed -i 's/^SMARTHOST=.*/SMARTHOST=10.1.1.1:2526/' /etc/vmspool.d/mail.conf; /etc/init.d/vmmaild restart; sleep 3; /etc/init.d/vmmaild status; netstat -ltn | grep ':25 '")
# mail in from the PC
m = MIMEText('Hello CMSUSER,\nthis came by SMTP from the PC to vmmaild in the Debian guest.\n\nMike')
m['From'] = 'Mike <mike@example.org>'; m['To'] = 'cmsuser@vm370plus.lan'; m['Subject'] = 'Mail from the PC'
try:
    s = smtplib.SMTP('10.1.1.2', 25, timeout=60); print('SMTP in:', s.sendmail('mike@example.org', ['cmsuser@vm370plus.lan'], m.as_string())); 
    try: s.sendmail('spam@example.org', ['victim@example.net'], 'Subject: x\n\nx\n'); print('relay accepted (from 10.x: trusted)')
    except Exception as e: print('relay refused:', e)
    s.quit()
except Exception as e: print('SMTP in failed:', repr(e))
time.sleep(40)
run('ls -l /var/spool/vmspool/outq /var/spool/vmspool/outq/failed 2>&1; vmcp q rdr cmsuser all 2>&1 | head; tail -5 /var/log/syslog /var/log/mail.log 2>/dev/null | grep -i "vmmaild\|vmspoold"')
print('PHASE1-DONE', flush=True)
# CMS side runs now; wait for its end marker
for i in range(400):
    if 'MAILTEST-END' in term(): break
    time.sleep(5)
time.sleep(10)
run('ls -l /var/spool/vmspool/outq /var/spool/vmspool/mail/* 2>&1 | head -20; grep -h "vmmaild\|vmspoold" /var/log/syslog /var/log/daemon.log /var/log/mail.log 2>/dev/null | tail -12; sync')
try: c.exec_command('sleep 2; halt', timeout=10)
except Exception: pass
c.close()
time.sleep(60)
urllib.request.urlopen('http://127.0.0.1:8081/cgi-bin/tasks/syslog?command=%2Fmsg%20operator%20MDDEB-DONE').read()
time.sleep(5)
