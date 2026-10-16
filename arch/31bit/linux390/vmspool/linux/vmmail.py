"""vmmail -- VM/370plus 9.18: mail between CMS users and the world.

Shared by vmmaild (the SMTP server: mail in) and the vmspool service
'mail' (SENDMAIL EXEC on CMS: mail out).  Python 3.2 (Debian 7).

Mail for a CMS user (userid@DOMAIN) becomes a reader file of that user,
named  sender MAIL , punched by vmspoold from its outbox
(/var/spool/vmspool/outq); GETMAIL EXEC on CMS reads it.  A copy is kept
in /var/spool/vmspool/mail/<userid>/.  Mail for other domains goes to the
SMARTHOST of /etc/vmspool.d/mail.conf, or straight to the domain's MX.

/etc/vmspool.d/mail.conf (KEY=value, # comments):
    DOMAIN=vm370plus.lan        the mail domain(s) of the CMS users
    SMARTHOST=                  host[:port] for other domains (empty: MX)
    SMARTUSER= SMARTPASS=       login at the smarthost, if it wants one
    SMARTTLS=no                 STARTTLS to the smarthost
    LISTEN=0.0.0.0:25           where vmmaild listens
"""
import email, email.utils, email.header, os, random, re, smtplib, socket, struct, textwrap, time

CONF = os.environ.get('VMMAIL_CONF', '/etc/vmspool.d/mail.conf')
SPOOL = os.environ.get('VMSPOOL_DIR', '/var/spool/vmspool')


def conf():
    c = {'DOMAIN': 'vm370plus.lan', 'SMARTHOST': '', 'SMARTUSER': '', 'SMARTPASS': '',
         'SMARTTLS': 'no', 'LISTEN': '0.0.0.0:25'}
    try:
        for l in open(CONF):
            l = l.split('#', 1)[0].strip()
            if '=' in l:
                k, v = l.split('=', 1)
                c[k.strip().upper()] = v.strip().strip('"')
    except IOError:
        pass
    c['DOMAINS'] = [d.lower() for d in c['DOMAIN'].split()]
    return c


USERID = re.compile(r'^[A-Za-z0-9@#$]{1,8}$')


def local_user(addr, c):
    """the VM userid an address delivers to, or None"""
    addr = addr.strip().strip('<>')
    if '@' not in addr:
        return addr.upper() if USERID.match(addr) else None
    lp, dom = addr.rsplit('@', 1)
    if dom.lower() in c['DOMAINS'] and USERID.match(lp):
        return lp.upper()
    return None


def hdr(v):
    """a header as plain text"""
    if v is None:
        return ''
    out = []
    for part, cs in email.header.decode_header(str(v)):
        if isinstance(part, bytes):
            part = part.decode(cs or 'latin-1', 'replace')
        out.append(part)
    return ' '.join(' '.join(out).split())


def text_of(msg):
    """the readable text of a message, and the attachments named"""
    body, extra = [], []
    for part in msg.walk():
        if part.is_multipart():
            continue
        ct = part.get_content_type()
        fn = part.get_filename()
        if ct == 'text/plain' and not fn:
            data = part.get_payload(decode=True) or b''
            body.append(data.decode(part.get_content_charset() or 'latin-1', 'replace'))
        elif ct == 'text/html' and not fn and not body:
            data = part.get_payload(decode=True) or b''
            h = data.decode(part.get_content_charset() or 'latin-1', 'replace')
            h = re.sub(r'(?is)<(script|style).*?</\1>', '', h)
            h = re.sub(r'(?i)<br\s*/?>|</p>|</div>|</tr>', '\n', h)
            body.append(re.sub(r'<[^>]+>', '', h).replace('&nbsp;', ' ').replace('&amp;', '&')
                        .replace('&lt;', '<').replace('&gt;', '>'))
        else:
            data = part.get_payload(decode=True) or b''
            extra.append('[attachment: %s, %s, %d bytes]' % (fn or '(no name)', ct, len(data)))
    return '\n'.join(body), extra


ASCII = {'—': '--', '–': '-', '‘': "'", '’': "'", '“': '"', '”': '"',
         '…': '...', '•': '*', ' ': ' ', '→': '->', '€': 'EUR'}


def cards(text, width=79):
    """lines for a CMS file: no line over width, characters CMS has"""
    out = []
    for l in text.replace('\r', '').split('\n'):
        l = ''.join(ASCII.get(ch, ch) for ch in l.expandtabs(8))
        l = l.encode('latin-1', 'replace').decode('latin-1').rstrip()
        if len(l) <= width:
            out.append(l)
        else:
            out.extend(textwrap.wrap(l, width, break_long_words=True, break_on_hyphens=False) or [''])
    return out


def deliver_local(user, msg, raw):
    """a message to a CMS user's reader (through vmspoold's outbox)"""
    fr = hdr(msg.get('From'))
    name = email.utils.parseaddr(fr)[1].split('@')[0] or 'MAIL'
    fn = re.sub(r'[^A-Z0-9@#$]', '', name.upper())[:8] or 'MAIL'
    body, extra = text_of(msg)
    lines = ['From:    ' + fr, 'To:      ' + hdr(msg.get('To'))]
    if msg.get('Cc'):
        lines.append('Cc:      ' + hdr(msg.get('Cc')))
    if msg.get('Date'):
        lines.append('Date:    ' + hdr(msg.get('Date')))
    lines += ['Subject: ' + hdr(msg.get('Subject')), '']
    lines += cards(body) + ([''] + extra if extra else [])
    stamp = '%d-%d-%d' % (time.time(), os.getpid(), random.randint(0, 99999))
    d = os.path.join(SPOOL, 'mail', user.lower())
    os.makedirs(d, exist_ok=True)
    with open(os.path.join(d, stamp + '.eml'), 'wb') as f:
        f.write(raw if isinstance(raw, bytes) else raw.encode('latin-1', 'replace'))
    if user == myid():          # the guest's own reader is vmspoold's: kept only
        return fn
    q = os.path.join(SPOOL, 'outq')
    os.makedirs(q, exist_ok=True)
    tmp = os.path.join(q, stamp + '.tmp')
    with open(tmp, 'w', encoding='latin-1', errors='replace') as f:
        f.write('TO %s CLASS A NAME %s MAIL\n' % (user, fn))
        f.write('\n'.join(lines) + '\n')
    os.rename(tmp, os.path.join(q, stamp + '.out'))
    return fn


_myid = []


def myid():
    """the userid of this Linux machine (vmcp q userid), '' if unknown"""
    if not _myid:
        try:
            import subprocess
            out = subprocess.Popen(['vmcp', 'q', 'userid'], stdout=subprocess.PIPE,
                                   stderr=open(os.devnull, 'w')).communicate()[0]
            _myid.append(out.decode('latin-1').split()[0].upper() if out.split() else '')
        except Exception:
            _myid.append('')
    return _myid[0]


def mx_hosts(domain):
    """the MX hosts of a domain, best first (a tiny resolver: resolv.conf's
    first nameserver, UDP); the domain itself when it has none"""
    ns = '127.0.0.1'
    try:
        for l in open('/etc/resolv.conf'):
            w = l.split()
            if len(w) >= 2 and w[0] == 'nameserver':
                ns = w[1]
                break
    except IOError:
        pass
    tid = random.randint(0, 65535)
    q = struct.pack('>HHHHHH', tid, 0x0100, 1, 0, 0, 0) + b''.join(
        bytes([len(l)]) + l.encode() for l in domain.split('.') if l) + b'\0' + struct.pack('>HH', 15, 1)
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.settimeout(10)
    try:
        s.sendto(q, (ns, 53))
        p = s.recv(1500)
    except (socket.error, socket.timeout):
        return [domain]
    finally:
        s.close()

    def name(o):
        out, end, jumped = [], None, False
        while True:
            c = p[o]
            if c >= 0xC0:
                if not jumped:
                    end = o + 2
                o, jumped = (c & 0x3F) << 8 | p[o + 1], True
                continue
            if c == 0:
                return '.'.join(out), (end if jumped else o + 1)
            out.append(p[o + 1:o + 1 + c].decode('latin-1'))
            o += c + 1
    an = struct.unpack('>H', p[6:8])[0]
    o = 12
    _, o = name(o)
    o += 4
    mx = []
    for _ in range(an):
        _, o = name(o)
        ty, cl, ttl, rl = struct.unpack('>HHIH', p[o:o + 10])
        o += 10
        if ty == 15:
            mx.append((struct.unpack('>H', p[o:o + 2])[0], name(o + 2)[0]))
        o += rl
    return [h for _, h in sorted(mx)] or [domain]


def relay(c, mailfrom, rcpts, data):
    """mail for other domains: to the smarthost, or to each domain's MX;
    returns [(recipient, 'ok ...' or 'failed ...')]"""
    out = []
    if c['SMARTHOST']:
        h, _, p = c['SMARTHOST'].partition(':')
        try:
            s = smtplib.SMTP(h, int(p or 25), timeout=60)
            if c['SMARTTLS'].lower() in ('yes', 'y', '1', 'true'):
                s.starttls()
            if c['SMARTUSER']:
                s.login(c['SMARTUSER'], c['SMARTPASS'])
            bad = s.sendmail(mailfrom, rcpts, data)
            s.quit()
            for r in rcpts:
                out.append((r, 'failed %s' % (bad[r],) if r in bad else 'ok via %s' % c['SMARTHOST']))
        except Exception as e:
            out += [(r, 'failed via %s: %s' % (c['SMARTHOST'], e)) for r in rcpts]
        return out
    bydom = {}
    for r in rcpts:
        bydom.setdefault(r.rsplit('@', 1)[-1].lower(), []).append(r)
    for dom, rs in bydom.items():
        err = 'no MX'
        for h in mx_hosts(dom):
            try:
                s = smtplib.SMTP(h, 25, timeout=60)
                bad = s.sendmail(mailfrom, rs, data)
                s.quit()
                out += [(r, 'failed %s' % (bad[r],) if r in bad else 'ok via %s' % h) for r in rs]
                break
            except Exception as e:
                err = '%s: %s' % (h, e)
        else:
            out += [(r, 'failed ' + err) for r in rs]
    return out
