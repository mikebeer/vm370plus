import sys, time, urllib.request, urllib.parse, paramiko
log = sys.argv[1]; D = '/home/claude/scratch/mcp'
def term():
    try: return open(log, errors='replace').read()
    except FileNotFoundError: return ''
def opmsg(t): urllib.request.urlopen('http://127.0.0.1:8081/cgi-bin/tasks/syslog?command=' + urllib.parse.quote('/msg operator ' + t)).read()
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
def run(cmd, t=3600):
    i, o, e = c.exec_command(cmd, timeout=t)
    out = o.read().decode(errors='replace') + e.read().decode(errors='replace')
    print('$ %s [rc %s]\n%s' % (cmd, o.channel.recv_exit_status(), out), flush=True)
s = c.open_sftp(); s.put(D + '/ticket.handler', '/etc/vmspool.d/ticket'); s.close()
for i in range(60):
    i, o, e = c.exec_command('/etc/init.d/mariadb status', timeout=300)
    if 'running' in o.read().decode(): break
    time.sleep(20)
s = c.open_sftp(); s.put(D + '/mcapps.cron', '/etc/cron.d/vm370plus-mcapps'); s.close()
run('chmod 644 /etc/cron.d/vm370plus-mcapps; chmod 755 /etc/vmspool.d/ticket; touch /var/log/mcp/worker.log /var/log/mchelpdesk/worker.log; chown mcp /var/log/mcp/worker.log; chown www-data /var/log/mchelpdesk/worker.log; /etc/init.d/cron reload')
opmsg('MDB-READY')
for i in range(1200):
    if 'MDB-END' in term(): break
    time.sleep(3)
for i in range(80):
    i_, o_, e_ = c.exec_command('test -s /var/log/mchelpdesk/worker.log -a -s /var/log/mcp/worker.log && echo yes', timeout=60)
    if 'yes' in o_.read().decode(): break
    time.sleep(15)
run('tail -5 /var/log/mchelpdesk/worker.log /var/log/mcp/worker.log; grep -i cron /var/log/syslog | tail -4')
run('mariadb mchelpdesk -e "select t.number, t.title, c.email, (select count(*) from articles a where a.ticket_id=t.id) arts from tickets t join contacts c on c.id=t.contact_id"; free')
try: c.exec_command('sleep 2; halt', timeout=10)
except Exception: pass
time.sleep(120)
opmsg('MDDEB-DONE')
