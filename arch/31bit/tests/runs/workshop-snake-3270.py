# ws4: SNAKE, SLASH, DISKPRY over s3270 with real waits (9.13e)
import sys, time, re, subprocess, urllib.request, urllib.parse
log = sys.argv[1]
def herc(cmd):
    urllib.request.urlopen('http://127.0.0.1:8081/cgi-bin/tasks/syslog?command=' + urllib.parse.quote(cmd)).read()
for i in range(600):
    try:
        if 'LOGOFF AS AUTOLOG1' in open(log.replace('.term.log', '.log'), errors='replace').read(): break
    except FileNotFoundError: pass
    time.sleep(3)
p = subprocess.Popen(['s3270', '-model', '3279-2'], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True, bufsize=1)
def act(cmd):
    p.stdin.write(cmd + '\n'); p.stdin.flush(); out = []
    while True:
        l = p.stdout.readline()
        if not l: break
        out.append(l.rstrip('\n'))
        if l.startswith('ok') or l.startswith('error'): break
    return out
def scr():
    return '\n'.join(l[6:] for l in act('Ascii()') if l.startswith('data: '))
def show(t):
    print('==== %s' % t); print('\n'.join(x.rstrip() for x in scr().split('\n') if x.strip())); sys.stdout.flush()
def status():
    return scr().split('\n')[-1]
def wait(rx, t=30):
    end = time.time() + t
    while time.time() < end:
        s = scr()
        if re.search(rx, s): return s
        if 'MORE...' in s.split('\n')[-1]: act('Clear()')
        time.sleep(1)
    return None
def ready(t=60):
    end = time.time() + t
    while time.time() < end:
        st = status()
        if 'MORE...' in st or 'HOLDING' in st: act('Clear()'); time.sleep(2); continue
        if 'VM READ' in st or 'CP READ' in st: return True
        time.sleep(1)
    return False
def cmd2(c, t=60):
    ready(10); act('String("%s")' % c); act('Enter()'); time.sleep(2); r = ready(t); print('---- %s -> %s' % (c, 'ok' if r else 'TIMEOUT')); sys.stdout.flush()
def cmd(c, rx=r'Ready|RUNNING', t=30):
    s = scr()
    if 'MORE...' in s.split('\n')[-1] or 'HOLDING' in s.split('\n')[-1]: act('Clear()'); time.sleep(3)
    act('Wait(5,InputField)'); act('String("%s")' % c); act('Enter()'); time.sleep(2)
    r = wait(rx, t); print('---- %s -> %s' % (c, 'ok' if r else 'TIMEOUT')); return r
act('Connect(127.0.0.1:3270)'); wait('VM/370|Online|ONLINE|Enter', 20)
act('Enter()'); time.sleep(3); act('Wait(5,InputField)')
act('String("logon cmsuser cmsuser")'); act('Enter()')
print('logon', 'ok' if wait('LOGON AT|Ready', 40) else 'TIMEOUT'); ready(60); show('logged on')
cmd2('listfile * module a'); show('modules')
cmd2('q term'); cmd2('q screen')
act('String("snake")'); act('Enter()')
for i in range(4):
    time.sleep(5); show('snake +%ds' % (5 * (i + 1)))
act('PF(6)'); time.sleep(4); show('snake PF6')
act('PA(1)'); time.sleep(4); show('snake PA1')
act('Enter()'); time.sleep(4); show('after')
ready(30); act('Clear()'); time.sleep(2)
cmd2('cp logoff', 10)
act('Disconnect()'); act('Quit()')
herc('/msg operator MDDEB-DONE')
