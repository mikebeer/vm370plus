#!/usr/bin/env python3
"""vm370plus-setup.py -- the VM/370plus setup wizard (9.45).

Installs VM/370plus from VM370CE_V1_R1_2.zip and the kit parts, on
Windows, Linux and macOS (Python 3.6 or later, nothing else needed):

    python3 vm370plus-setup.py                 ask everything
    python3 vm370plus-setup.py --yes           take the defaults
    python3 vm370plus-setup.py --answers a.json [--yes]
    python3 vm370plus-setup.py --apply DIR     send the guest settings
                                               (mail, AI, NJE) to the
                                               running system's Debian

What it does:
  1. finds CE's zip and the newest complete set of kit parts (in the
     directory of this script, the current directory, or where you say);
  2. checks for Python and Hercules (hercules on the PATH, or tell it
     where);
  3. unpacks CE into a new directory and every kit part over it, and
     joins the split disks (Debian, language), checking their SHA-256;
  4. asks for the settings and writes them into vm370ce.conf: storage
     (MAINSIZE), the 3270 port, the CTC to the PC (Linux networking:
     ssh, NJE, mail from outside -- needs a TUN device and root/admin
     rights for Hercules);
  5. keeps the guest settings (mail domain and smarthost, the AI model
     server, the NJE node and peers) in vm370plus-settings.json, which
     --apply sends to the Debian guest once the system runs (the vmspool
     service 'setup', through the PC card reader 010C, tools/vmspool.py).

Nothing outside the new directory is changed.  Answers go into
vm370plus-settings.json next to vm370ce.conf; run the wizard again with
--answers to repeat an installation.
"""
import argparse, glob, hashlib, json, os, re, shutil, subprocess, sys, zipfile

CE_ZIP = 'VM370CE_V1_R1_2.zip'
TOP = 'VM370CE.V1.R1.2'

DEFAULTS = {
    'dir': '',                    # where to install (a new directory)
    'mainsize': 256,              # MB of real storage for CP (16..2048)
    'cnslport': 3270,             # tn3270 port
    'ctc': False,                 # the CTC pair to the PC (Linux networking)
    'mail_domain': 'vm370plus.lan',
    'smarthost': '',              # host[:port] for outgoing mail
    'smartuser': '', 'smartpass': '', 'smarttls': 'no',
    'ai_url': 'http://10.1.1.1:8080/v1/chat/completions',  # OpenAI protocol
    'ai_model': '',
    'ai_key': '',
    'nje_node': 'VM370CE',
    'nje_peers': 'PCNJE:10.1.1.1',         # NAME:host[:port] ...
}

QUESTIONS = [
    ('dir', 'Install into which (new) directory'),
    ('mainsize', 'Real storage for VM/370 in MB (256 is the tested size; 16 makes paging space run out)'),
    ('cnslport', 'Port for the 3270 terminals'),
    ('ctc', 'Turn on the network link to the Debian guest (ssh, NJE, mail from outside; needs TUN and admin rights)'),
    ('mail_domain', 'Mail domain of the CMS users'),
    ('smarthost', 'Smarthost for outgoing mail, host[:port] (empty: send directly)'),
    ('smartuser', 'Smarthost login (empty: none)'),
    ('smartpass', 'Smarthost password'),
    ('smarttls', 'STARTTLS to the smarthost (yes/no)'),
    ('ai_url', 'AI model server (OpenAI protocol), or "anthropic"'),
    ('ai_model', 'Model name (empty: the server\'s default)'),
    ('ai_key', 'API key (empty: none)'),
    ('nje_node', 'NJE node name of this system (up to 8 characters)'),
    ('nje_peers', 'NJE peers, NAME:host[:port] separated by blanks'),
]


def say(*a):
    print(*a, flush=True)


def ask(key, prompt, cur, yes):
    if yes:
        return cur
    if isinstance(cur, bool):
        d = 'y' if cur else 'n'
        while True:
            v = input('%s? [%s] ' % (prompt, d)).strip().lower() or d
            if v[:1] in ('y', 'n'):
                return v[:1] == 'y'
    v = input('%s [%s] ' % (prompt, cur)).strip()
    if not v:
        return cur
    if isinstance(cur, int):
        try:
            return int(v)
        except ValueError:
            say('  a number, please'); return ask(key, prompt, cur, yes)
    return v


def find_inputs(where):
    """CE's zip and the newest complete kit (list of part files)."""
    dirs = [d for d in where if d and os.path.isdir(d)]
    ce = None
    for d in dirs:
        p = os.path.join(d, CE_ZIP)
        if os.path.isfile(p):
            ce = p; break
    kits = {}
    for d in dirs:
        for p in glob.glob(os.path.join(d, 'VM370PLUS-kit-*-b*-part*.zip')):
            m = re.search(r'-b(\d+)-part(\d+)\.zip$', p)
            if m:
                kits.setdefault(int(m.group(1)), {})[int(m.group(2))] = p
    best = None
    for b in sorted(kits, reverse=True):
        parts = kits[b]
        if parts and sorted(parts) == list(range(1, max(parts) + 1)):
            best = (b, [parts[i] for i in sorted(parts)]); break
        say('kit build %d is incomplete (parts %s), skipped' % (b, sorted(parts)))
    return ce, best


def hercules():
    for name in ('hercules', 'hercules.exe'):
        p = shutil.which(name)
        if p:
            try:
                out = subprocess.run([p, '--version'], stdout=subprocess.PIPE,
                                     stderr=subprocess.STDOUT, timeout=20).stdout.decode('latin-1', 'replace')
            except Exception:
                out = ''
            m = re.search(r'[Vv]ersion\s+([0-9][0-9.]*)', out)
            return p, (m.group(1) if m else '?')
    return None, None


def unpack(ce, parts, dest):
    os.makedirs(dest, exist_ok=True)
    say('unpacking %s ...' % os.path.basename(ce))
    zipfile.ZipFile(ce).extractall(dest)
    for p in parts:
        say('unpacking %s ...' % os.path.basename(p))
        zipfile.ZipFile(p).extractall(dest)
    root = os.path.join(dest, TOP)
    if not os.path.isfile(os.path.join(root, 'vm370ce.conf')):
        sys.exit('### %s has no vm370ce.conf: not CE?' % root)
    # the pieces of the split disks
    disks = os.path.join(root, 'disks')
    for base in sorted({re.sub(r'\.\d{3}$', '', f) for f in os.listdir(disks) if re.search(r'\.cckd\.\d{3}$', f)}):
        pieces = sorted(f for f in os.listdir(disks) if f.startswith(base + '.') and re.search(r'\.\d{3}$', f))
        out = os.path.join(disks, base)
        say('joining %s from %d pieces ...' % (base, len(pieces)))
        h = hashlib.sha256()
        with open(out, 'wb') as o:
            for f in pieces:
                with open(os.path.join(disks, f), 'rb') as i:
                    while True:
                        b = i.read(1 << 20)
                        if not b:
                            break
                        o.write(b); h.update(b)
        sha = os.path.join(disks, base.replace('.cckd', '') + '.sha256')
        if os.path.isfile(sha):
            want = open(sha).read().split()[0].lower()
            if want != h.hexdigest():
                sys.exit('### %s: SHA-256 does not match %s' % (out, sha))
            say('  %s: SHA-256 ok' % base)
        for f in pieces:
            os.remove(os.path.join(disks, f))
    return root


def edit_conf(root, a):
    p = os.path.join(root, 'vm370ce.conf')
    s = open(p, encoding='latin-1').read()
    s = re.sub(r'(?m)^MAINSIZE\s+\d+', 'MAINSIZE        %d' % a['mainsize'], s)
    s = re.sub(r'(?m)^CNSLPORT\s+\d+', 'CNSLPORT        %d' % a['cnslport'], s)
    if a['ctc']:
        s = re.sub(r'(?m)^#(0600\.2\s+CTCI)', r'\1', s)
    else:
        s = re.sub(r'(?m)^(0600\.2\s+CTCI)', r'#\1', s)
    open(p, 'w', encoding='latin-1').write(s)
    say('vm370ce.conf: MAINSIZE %d, CNSLPORT %d, CTC to the PC %s'
        % (a['mainsize'], a['cnslport'], 'on' if a['ctc'] else 'off'))


def settings_cards(a):
    """the request for the Debian guest's 'setup' service: KEY=value lines"""
    keys = ['mail_domain', 'smarthost', 'smartuser', 'smartpass', 'smarttls',
            'ai_url', 'ai_model', 'ai_key', 'nje_node', 'nje_peers']
    lines = ['setup FROM OPERATOR APPLY']
    for k in keys:                      # cards: 80 columns, so long
        v = str(a.get(k, ''))           # values go on as KEY+=more
        if k == 'nje_peers':            # one peer a card (vmspoold cuts
            ps = v.split() or ['']      # trailing blanks, not leading)
            lines.append('NJE_PEERS=%s' % ps[0])
            lines += ['NJE_PEERS+= %s' % x for x in ps[1:]]
            continue
        lines.append('%s=%s' % (k.upper(), v[:60]))
        for i in range(60, len(v), 60):
            lines.append('%s+=%s' % (k.upper(), v[i:i + 60]))
    return '\n'.join(lines) + '\n'


def apply(root, a):
    tool = os.path.join(root, 'vm370plus', 'tools', 'vmspool.py')
    if not os.path.isfile(tool):
        sys.exit('### no %s' % tool)
    f = os.path.join(root, 'vm370plus-settings.cards')
    open(f, 'w').write(settings_cards(a))
    say('sending the guest settings to MAINT (the Debian guest) ...')
    r = subprocess.run([sys.executable, tool, f, 'MAINT', 'SETUP', 'REQUEST'])
    os.remove(f)
    if r.returncode:
        sys.exit('### vmspool.py failed (is VM/370plus up, with RDR 010C started?)')
    say('sent; the answer (VMSPOOL REPLY) goes to OPERATOR\'s reader')


def main():
    ap = argparse.ArgumentParser(description='VM/370plus setup wizard (9.45)')
    ap.add_argument('--yes', action='store_true', help='take the defaults (or the answers file)')
    ap.add_argument('--answers', help='a JSON file of answers (keys as in vm370plus-settings.json)')
    ap.add_argument('--from', dest='src', action='append', default=[],
                    help='a directory with CE\'s zip and the kit parts (more than one allowed)')
    ap.add_argument('--apply', metavar='DIR', help='send the guest settings of the installation in DIR')
    ap.add_argument('--hercules', help='the hercules program, if it is not on the PATH')
    o = ap.parse_args()
    a = dict(DEFAULTS)
    if o.answers:
        a.update(json.load(open(o.answers)))

    if o.apply:
        root = o.apply if os.path.basename(os.path.normpath(o.apply)) == TOP else os.path.join(o.apply, TOP)
        sf = os.path.join(root, 'vm370plus-settings.json')
        if os.path.isfile(sf):
            a.update(json.load(open(sf)))
        if o.answers:                       # the answers file wins
            a.update(json.load(open(o.answers)))
        apply(root, a)
        return

    say('VM/370plus setup (9.45)\n')
    say('Python %s on %s' % (sys.version.split()[0], sys.platform))
    hp, hv = (o.hercules, '?') if o.hercules else hercules()
    if hp:
        say('Hercules: %s (version %s)' % (hp, hv))
        if hv.startswith('3.') and hv != '3.13':
            say('  note: tested with Hercules 4.x and 3.13')
    else:
        say('Hercules: not found on the PATH -- install SDL Hyperion 4.x (or 3.13)\n'
            '  before you start VM/370plus; the installation itself does not need it')
    here = os.path.dirname(os.path.abspath(__file__))
    ce, kit = find_inputs(o.src + [here, os.getcwd(), os.path.join(os.path.expanduser('~'), 'Downloads')])
    if not ce:
        ce = ask('ce', 'Where is %s' % CE_ZIP, '', o.yes)
        if not ce or not os.path.isfile(ce):
            sys.exit('### %s not found' % CE_ZIP)
    if not kit:
        sys.exit('### no complete set of kit parts (VM370PLUS-kit-...-bNN-partN.zip) found;\n'
                 '    put them next to this script or name the directory with --from')
    say('CE: %s\nkit: build %d, %d parts' % (ce, kit[0], len(kit[1])))
    if not a['dir']:
        a['dir'] = os.path.join(os.getcwd(), 'vm370plus')
    say('')
    for k, q in QUESTIONS:
        a[k] = ask(k, q, a[k], o.yes)
    a['mainsize'] = max(16, min(2048, int(a['mainsize'])))
    a['nje_node'] = str(a['nje_node']).upper()[:8]
    dest = os.path.abspath(a['dir'])
    if os.path.exists(os.path.join(dest, TOP)):
        sys.exit('### %s already exists: choose a new directory' % os.path.join(dest, TOP))
    root = unpack(ce, kit[1], dest)
    edit_conf(root, a)
    a['kit_build'] = kit[0]
    json.dump(a, open(os.path.join(root, 'vm370plus-settings.json'), 'w'), indent=1)
    start = 'vm370ce.cmd' if os.name == 'nt' else 'sh vm370ce.sh'
    say('\nVM/370plus is installed in %s' % root)
    say('Start it there with  %s  and connect a tn3270 emulator to localhost:%d.' % (start, a['cnslport']))
    say('The first IPL warm-starts by itself (if CP asks, answer  /cold ).')
    if a['ctc']:
        say('The CTC to the PC is on: Hercules needs a TUN device and admin rights (Linux: run\n'
            'as root or give hercifc its capabilities; Windows: TunTap and CTCI-WIN).')
    say('When the Debian guest runs (README: START LINUX FROM ITS DISK), send it the mail,\n'
        'AI and NJE settings with\n    python3 %s --apply %s' % (os.path.abspath(__file__), dest))


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        sys.exit('\ncancelled')
