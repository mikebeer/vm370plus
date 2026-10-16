"""vmask -- VM/370plus 9.10: the AI gateway.  Questions from CMS (ASK EXEC,
over the spool bridge) and from Linux (ask) go to a language model: a
llama.cpp / Ollama / LM Studio server on the PC, or a cloud API.

/etc/vmspool.d/ask.conf (KEY=value, # comments):
    API=openai                  openai (chat completions: llama.cpp,
                                Ollama, LM Studio, OpenAI, ...) or anthropic
    URL=http://10.1.1.1:8080/v1/chat/completions
    MODEL=local                 the model name the server wants
    APIKEY=                     sent as Bearer (openai) or x-api-key
    MAXTOKENS=800
    TIMEOUT=300
    HISTORY=6                   earlier questions and answers kept per user
    SYSTEM=You are the help desk of VM/370plus ...
Python 3.2 (Debian 7).
"""
import json, os, textwrap, time, urllib.request, urllib.error

CONF = os.environ.get('VMASK_CONF', '/etc/vmspool.d/ask.conf')
STATE = os.environ.get('VMASK_DIR', '/var/spool/vmspool/ask')
DEFAULT_SYSTEM = ('You are the help desk of VM/370plus, a VM/370 system (CP and CMS) '
                  'with Linux guests. Answer briefly and plainly; the answer is read '
                  'on an 80-column terminal, so use no Markdown tables.')


def conf():
    c = {'API': 'openai', 'URL': 'http://10.1.1.1:8080/v1/chat/completions', 'MODEL': 'local',
         'APIKEY': '', 'MAXTOKENS': '800', 'TIMEOUT': '300', 'HISTORY': '6', 'SYSTEM': DEFAULT_SYSTEM}
    try:
        for l in open(CONF):
            l = l.strip()
            if l.startswith('#') or '=' not in l:
                continue
            k, v = l.split('=', 1)
            c[k.strip().upper()] = v.strip()
    except IOError:
        pass
    return c


def history(user):
    try:
        return json.load(open(os.path.join(STATE, user.lower() + '.json')))
    except (IOError, ValueError):
        return []


def save(user, h):
    if not os.path.isdir(STATE):
        os.makedirs(STATE)
    json.dump(h, open(os.path.join(STATE, user.lower() + '.json'), 'w'))


def forget(user):
    try:
        os.remove(os.path.join(STATE, user.lower() + '.json'))
    except OSError:
        pass


def ask(user, question, c=None):
    """the answer to question (with the user's earlier exchanges)"""
    c = c or conf()
    keep = int(c['HISTORY'])
    h = history(user)[-2 * keep:] if keep > 0 else []
    msgs = h + [{'role': 'user', 'content': question}]
    if c['API'].lower() == 'anthropic':
        body = {'model': c['MODEL'], 'max_tokens': int(c['MAXTOKENS']), 'system': c['SYSTEM'],
                'messages': msgs}
        hdr = {'Content-Type': 'application/json', 'x-api-key': c['APIKEY'],
               'anthropic-version': '2023-06-01'}
    else:
        body = {'model': c['MODEL'], 'max_tokens': int(c['MAXTOKENS']),
                'messages': [{'role': 'system', 'content': c['SYSTEM']}] + msgs}
        hdr = {'Content-Type': 'application/json'}
        if c['APIKEY']:
            hdr['Authorization'] = 'Bearer ' + c['APIKEY']
    req = urllib.request.Request(c['URL'], json.dumps(body).encode('utf-8'), hdr)
    t0 = time.time()
    try:
        r = json.loads(urllib.request.urlopen(req, timeout=int(c['TIMEOUT'])).read().decode('utf-8'))
    except urllib.error.HTTPError as e:
        raise RuntimeError('%s answered %d: %s' % (c['URL'], e.code, e.read().decode('utf-8', 'replace')[:300]))
    except Exception as e:
        raise RuntimeError('%s: %s' % (c['URL'], e))
    if c['API'].lower() == 'anthropic':
        text = ''.join(b.get('text', '') for b in r.get('content', []) if b.get('type') == 'text')
    else:
        text = r['choices'][0]['message']['content']
    text = text.strip()
    if keep > 0:
        save(user, (h + [{'role': 'user', 'content': question},
                         {'role': 'assistant', 'content': text}])[-2 * keep:])
    return text, time.time() - t0


ASCII = {'—': '--', '–': '-', '‘': "'", '’': "'", '“': '"', '”': '"',
         '…': '...', '•': '*', ' ': ' ', '→': '->', '≤': '<=', '≥': '>='}


def lines(text, width=79):
    """the answer for an 80-column terminal"""
    out = []
    for l in text.replace('\r', '').split('\n'):
        l = ''.join(ASCII.get(ch, ch) for ch in l.expandtabs(4)).rstrip()
        l = l.encode('latin-1', 'replace').decode('latin-1')
        if len(l) <= width:
            out.append(l)
        else:
            ind = len(l) - len(l.lstrip())
            sub = ' ' * (ind + 2 if l.lstrip()[:2] in ('- ', '* ') else ind)
            out.extend(textwrap.wrap(l, width, subsequent_indent=sub, break_on_hyphens=False))
    return out
