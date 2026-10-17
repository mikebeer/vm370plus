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
run('chmod 755 /etc/vmspool.d/ticket; export PATH=/usr/local/regina39/bin:/usr/local/mariadb/bin:$PATH; HD_PASSWORD=VM370plus-Maint-1 HD_CONF=/etc/mchelpdesk/mchelpdesk.conf hd-admin tenant-create vm370plus maint maint@vm370plus.lan VM/370plus 2>&1', 1800)
opmsg('MDB-READY')
for i in range(1200):
    if 'MDB-END' in term(): break
    time.sleep(3)
run('mariadb mchelpdesk -e "select t.number, t.title, c.email, (select count(*) from articles a where a.ticket_id=t.id) arts from tickets t join contacts c on c.id=t.contact_id"; free')
try: c.exec_command('sleep 2; halt', timeout=10)
except Exception: pass
time.sleep(120)
opmsg('MDDEB-DONE')
