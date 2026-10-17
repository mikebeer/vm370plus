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
  2. checks for Python, Hercules and a tn3270 emulator, and offers to
     install what is missing (asking first): Hercules on Linux by building
     the newest SDL Hyperion with hercules-helper (it is rarely prebuilt),
     or Homebrew / the distribution's package; on macOS with Homebrew; on
     Windows it shows where to get it (WC3270 comes with CE);
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


HELPER = 'https://github.com/wrljet/hercules-helper.git'
HYPERION = 'https://github.com/SDL-Hercules-390/hyperion'


def confirm(prompt, yes, default=True):
    if yes:
        return default
    d = 'Y/n' if default else 'y/N'
    v = input('%s [%s] ' % (prompt, d)).strip().lower()
    return default if not v else v[:1] == 'y'


def run_shown(cmd, cwd=None):
    say('  $ ' + (cmd if isinstance(cmd, str) else ' '.join(cmd)))
    return subprocess.call(cmd, cwd=cwd, shell=isinstance(cmd, str))


def pkg_manager():
    """the package manager of this Linux, and how it installs a package"""
    for pm, cmd in (('apt-get', 'sudo apt-get install -y %s'), ('dnf', 'sudo dnf install -y %s'),
                    ('zypper', 'sudo zypper install -y %s'), ('pacman', 'sudo pacman -S --noconfirm %s')):
        if shutil.which(pm):
            return pm, cmd
    return None, None


def find_built_hercules():
    """a hercules that hercules-helper built (it installs under ~/herctest)"""
    home = os.path.expanduser('~')
    for pat in ('herctest/*/bin/hercules', 'herctest/bin/hercules', 'hercules-build/*/bin/hercules',
                'herctest/*/*/bin/hercules'):
        for p in sorted(glob.glob(os.path.join(home, pat))):
            if os.access(p, os.X_OK):
                return p
    return None


def install_hercules(yes):
    """offer to install Hercules; returns the program's path or None.
    The newest SDL Hyperion is rarely prebuilt for Linux: the dependable
    way there is to build it with hercules-helper (it installs the build
    packages itself, asking for sudo).  macOS: Homebrew has 4.9.x bottles.
    Windows: SDL Hyperion's own releases (or hercules-helper-windows)."""
    say('\nHercules is needed to run VM/370plus (SDL Hyperion 4.x, or 3.13).')
    if sys.platform == 'darwin':
        if shutil.which('brew'):
            if confirm('Install it with Homebrew (brew install hercules)?', yes):
                if run_shown(['brew', 'install', 'hercules']) == 0:
                    return shutil.which('hercules')
        else:
            say('  Homebrew is not installed (https://brew.sh); with it: brew install hercules')
    elif os.name == 'nt':
        say('  Windows: get the newest SDL Hyperion from %s/releases\n'
            '  (or build it with https://github.com/wrljet/hercules-helper-windows),\n'
            '  then run this wizard again, or give the path with --hercules.' % HYPERION)
        if confirm('Open the SDL Hyperion releases page in your browser?', yes, default=False):
            import webbrowser
            webbrowser.open(HYPERION + '/releases')
        return None
    else:
        choices = []
        if shutil.which('git'):
            choices.append(('build', 'build the newest SDL Hyperion from source with hercules-helper '
                            '(about 15 minutes; installs build tools with sudo; not as root)'))
        if shutil.which('brew'):
            choices.append(('brew', 'brew install hercules (Homebrew on Linux, prebuilt 4.9.x)'))
        pm, pcmd = pkg_manager()
        if pm:
            choices.append(('pkg', '%s (the distribution\'s package: often older, e.g. 3.13)' % (pcmd % 'hercules')))
        if not choices:
            say('  no git, Homebrew or package manager found: install git and run this again')
            return None
        for i, (k, t) in enumerate(choices, 1):
            say('  %d. %s' % (i, t))
        say('  0. skip (install Hercules yourself)')
        c = '1' if yes else (input('Your choice [1] ').strip() or '1')
        if not c.isdigit() or not 0 < int(c) <= len(choices):
            return None
        k = choices[int(c) - 1][0]
        if k == 'brew':
            if run_shown(['brew', 'install', 'hercules']) == 0:
                return shutil.which('hercules')
        elif k == 'pkg':
            if run_shown(pcmd % 'hercules') == 0:
                return shutil.which('hercules')
        else:
            if os.geteuid() == 0:
                say('  hercules-helper must not run as root; run the wizard as your own user')
                return None
            base = os.path.join(os.path.expanduser('~'), 'hercules-build')
            os.makedirs(base, exist_ok=True)
            hh = os.path.join(base, 'hercules-helper')
            if not os.path.isdir(hh):
                if run_shown(['git', 'clone', HELPER, hh]):
                    return None
            work = os.path.join(base, 'work')
            os.makedirs(work, exist_ok=True)
            if run_shown([os.path.join(hh, 'hercules-buildall.sh'), '--flavor=sdl-hyperion', '--auto'], cwd=work):
                say('  the build failed; its log is in %s' % work)
                return None
            p = shutil.which('hercules') or find_built_hercules()
            if p:
                say('  built: %s' % p)
                say('  (hercules-helper writes a setvars script next to the build; source it, or put\n'
                    '   %s on your PATH, before you start VM/370plus)' % os.path.dirname(p))
            return p
    return None


def tn3270(yes):
    """a 3270 emulator: CE brings WC3270 for Windows; elsewhere c3270/x3270"""
    if os.name == 'nt' or shutil.which('c3270') or shutil.which('x3270'):
        return
    say('\nNo tn3270 emulator found (c3270 or x3270).')
    if sys.platform == 'darwin' and shutil.which('brew'):
        if confirm('Install c3270 with Homebrew (brew install x3270)?', yes):
            run_shown(['brew', 'install', 'x3270'])
        return
    pm, pcmd = pkg_manager()
    if pm:
        pkg = 'c3270' if pm == 'apt-get' else 'x3270'
        if confirm('Install it (%s)?' % (pcmd % pkg), yes):
            run_shown(pcmd % pkg)
    else:
        say('  install c3270 or x3270 (https://x3270.miraheze.org) to connect to localhost:3270')


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
    ap.add_argument('--no-install', action='store_true',
                    help='do not offer to install missing prerequisites (Hercules, a tn3270 emulator)')
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
        say('Hercules: not found on the PATH')
        if o.no_install or o.yes:
            say('  (install SDL Hyperion 4.x or 3.13 before you start VM/370plus;\n'
                '   run the wizard without --yes to have it installed)')
        else:
            hp = install_hercules(o.yes)
            if not hp:
                say('  the installation goes on; install Hercules before you start VM/370plus')
    if not (o.no_install or o.yes):
        tn3270(o.yes)
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
    except (KeyboardInterrupt, EOFError):
        sys.exit('\ncancelled')
