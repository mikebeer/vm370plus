import sys, time, urllib.request, urllib.parse, paramiko, os
log = sys.argv[1]
D = '/home/claude/scratch/mdb'
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
def run(cmd, t=3600):
    t0 = time.time()
    i, o, e = c.exec_command(cmd, timeout=t)
    out = o.read().decode(errors='replace'); err = e.read().decode(errors='replace')
    rc = o.channel.recv_exit_status()
    print('$ %s   [rc %s, %.0fs]\n%s%s' % (cmd, rc, time.time() - t0, out, err), flush=True)
    return rc
run('mkdir -p /root/mdb; df -h / | tail -1; free')
s = c.open_sftp()
for f in ['mariadb-s390.tgz', 'my.cnf', 'mariadb.init', 'install.sh', 'mcsql.tgz']:
    t0 = time.time(); s.put(os.path.join(D, f), '/root/mdb/' + f); print('put', f, '%.0fs' % (time.time() - t0), flush=True)
s.close()
run('cd /root/mdb && sh install.sh 2>&1 | tail -30', 5400)
run('free; ps -o pid,rss,vsz,comm -C mariadbd; cat /var/lib/mariadb/mariadb.err | tail -15')
run('cd /root/mdb && tar xzf mcsql.tgz && mariadb -e "create database mcp character set utf8mb4 collate utf8mb4_unicode_ci" && '
    'for f in 01_schema 02_routines_views 03_seed_system 05_seed_reports; do echo == $f; mariadb mcp < mcproject/sql/$f.sql || exit 1; done; '
    'sed -e s/@APP_PW@/a1/ -e s/@REPORT_PW@/r1/ -e s/@ADMIN_PW@/d1/ mcproject/sql/04_users.sql | mariadb mcp; echo rc04=$?', 3600)
run('cd /root/mdb && for f in 001_schema 002_functions 003_views 004_seed 005_dbusers; do echo == $f; mariadb < mchelpdesk/sql/$f.sql || exit 1; done', 3600)
run('mariadb -N -e "select table_schema, count(*) from information_schema.tables where table_schema in (\'mcp\',\'mchelpdesk\') group by 1; select routine_schema, routine_type, count(*) from information_schema.routines group by 1,2; select version()"')
run('free; ps -o pid,rss,vsz,comm -C mariadbd')
try: c.exec_command('sleep 2; halt', timeout=10)
except Exception: pass
time.sleep(120)
urllib.request.urlopen('http://127.0.0.1:8081/cgi-bin/tasks/syslog?command=' + urllib.parse.quote('/msg operator MDDEB-DONE')).read()
