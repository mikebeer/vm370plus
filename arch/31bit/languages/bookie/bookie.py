#!/usr/bin/env python3
"""bookie -- a BookMaster-compatible formatter for VM/370plus (9.30)

IBM's BookMaster is licensed; this is our own implementation of its GML
markup, enough for manuals, memos and README-style books.  It translates a
BookMaster source into troff (ms macros, tbl for tables), which groff
formats as text, PostScript or (with Ghostscript) PDF.

    bookie.py [-o out.roff] [-I dir] file.script     -> troff on stdout
    bookie.py --groff text|ps file.script            -> formatted output

Supported markup
  structure  :userdoc :prolog :title :author :date :docnum :frontm
             :tipage :toc :body :appendix :backm :index (ignored)
             :h0-:h6 (id= ignored), :p :lp :pc :note :hp0-:hp3 :ehpN
             :q :eq :cit :ecit :fn :efn :xph :exph
  lists      :ol :ul :sl :li :eol :eul :esl  (nested), :dl tsize= :dthd
             :ddhd :dt :dd :edl, :gl :gt :gd :egl
  blocks     :xmp :exmp, :lines :elines, :fig :figcap :efig,
             :table cols='...' :thd :ethd :row :c :etable, :hdref/:figref
             (text of the reference id, if known)
  control    .* comment, .sp [n], .br, .pa, .ce [n], .im name (imbed)
  symbols    &colon. &semi. &period. &amp. &gml. &lt. &gt. &rbl. &bullet.
             &dash. &mdash. &copy. &tm. &reg. &quot. &apos. &slash.
             &rbl. ... (and any &name. a :nameit. defined)
"""
import os
import re
import sys

SYMBOLS = {
    'colon': ':', 'semi': ';', 'period': '.', 'amp': '&', 'gml': ':',
    'lt': '<', 'gt': '>', 'rbl': '\\ ', 'bullet': '\\(bu', 'dash': '\\(en',
    'mdash': '\\(em', 'endash': '\\(en', 'copy': '\\(co', 'copyr': '\\(co',
    'tm': '\\(tm', 'reg': '\\(rg', 'quot': '"', 'apos': "'", 'slash': '/',
    'bslash': '\\e', 'lbrace': '{', 'rbrace': '}', 'lbrk': '[', 'rbrk': ']',
    'vbar': '|', 'caret': '^', 'tilde': '~', 'odq': '\\(lq', 'cdq': '\\(rq',
    'osq': '`', 'csq': "'", 'degree': '\\(de', 'mu': '\\(*m', 'eq': '=',
    'plus': '+', 'minus': '\\-', 'divide': '\\(di', 'times': '\\(mu',
    'ge': '\\(>=', 'le': '\\(<=', 'ne': '\\(!=', 'sterling': '\\(Po',
    'yen': '\\(Ye', 'cent': '\\(ct', 'dollar': '$', 'pound': '#',
    'percent': '%', 'atsign': '@', 'us': '_', 'emdash': '\\(em',
}

BLOCK = set('''userdoc euserdoc prolog eprolog title author date docnum
frontm tipage toc body appendix backm index h0 h1 h2 h3 h4 h5 h6 p lp pc
note ol eol ul eul sl esl li dl edl dthd ddhd dt dd gl egl gt gd xmp exmp
lines elines fig efig figcap table etable thd ethd row c fn efn docprof
abstract eabstract preface epreface address eaddress aline prodname subtitle
version based filenum stitle edition eedition attention'''.split())
INLINE = set('''hp0 hp1 hp2 hp3 hp4 hp5 hp6 hp7 hp8 hp9 ehp0 ehp1 ehp2 ehp3
ehp4 ehp5 ehp6 ehp7 ehp8 ehp9 q eq cit ecit xph exph hdref figref
nameit'''.split())
TAG = re.compile(r':([a-zA-Z][a-zA-Z0-9]*)((?:\s+[a-zA-Z]+(?:=(?:\'[^\']*\'|"[^"]*"|[^\s.]+))?)*)\s*\.')
HP = {'0': '\\fR', '1': '\\fI', '2': '\\fB', '3': '\\f(BI', '4': '\\fI',
      '5': '\\fB', '6': '\\f(BI', '7': '\\fI', '8': '\\fB', '9': '\\f(BI'}


def attrs(s):
    out = {}
    for m in re.finditer(r'([a-zA-Z]+)(?:=(\'[^\']*\'|"[^"]*"|\S+))?', s or ''):
        v = m.group(2) or ''
        if v[:1] in '\'"':
            v = v[1:-1]
        out[m.group(1).lower()] = v
    return out


def esc(text):
    """plain text into troff: backslashes, and nothing is a request"""
    return text.replace('\\', '\\e')


class Bookie:
    def __init__(self, incdirs):
        self.out = []
        self.inc = incdirs
        self.title = []; self.author = []; self.date = []; self.docnum = []
        self.where = None          # collecting prolog fields
        self.lists = []            # [kind, counter]
        self.dl = []               # pending :dt. text per :dl.
        self.verbatim = 0
        self.table = None          # {'cols': n, 'rows': [], 'cur': None, 'head': set()}
        self.toc = False
        self.ids = {}              # id -> heading text
        self.fig = 0
        self.para = False
        self.heading = None        # [level, text] collected until line end
        self.dtterm = None
        self.names = {}

    # ---- output helpers -------------------------------------------------
    def req(self, line):
        self.flush_heading()
        self.out.append(line)

    def text(self, t):
        if not t.strip():
            return
        if self.heading is not None:
            self.heading[1] += (' ' if self.heading[1] else '') + t.strip()
            return
        if self.where:
            getattr(self, self.where).append(t.strip())
            return
        if self.dtterm is not None:
            self.dtterm += t
            return
        if self.table is not None:
            if self.table['cur'] is not None and self.table['cur']:
                self.table['cur'][-1] += (' ' if self.table['cur'][-1] else '') + t.strip()
            return
        t = t.strip() if not self.verbatim else t
        if t[:1] in ('.', "'"):
            t = '\\&' + t
        self.out.append(t)

    def flush_heading(self):
        if self.heading is None:
            return
        lvl, txt = self.heading
        self.heading = None
        self.out.append('.NH %d' % lvl)
        self.out.append(txt)
        if self.toc:
            self.out.append('.XS')
            self.out.append('%s%s' % ('    ' * (lvl - 1), txt))
            self.out.append('.XE')
        self.out.append('.PP')

    def symbols(self, s):
        def rep(m):
            n = m.group(1).lower()
            if n in self.names:
                return self.names[n]
            return SYMBOLS.get(n, m.group(0))
        return re.sub(r'&([a-zA-Z][a-zA-Z0-9]*)\.', rep, s)

    # ---- tags ---------------------------------------------------------------
    def tag(self, name, a):
        n = name.lower()
        if n in ('h0', 'h1', 'h2', 'h3', 'h4', 'h5', 'h6'):
            self.close_lists()
            lvl = max(1, int(n[1]))
            self.heading = [lvl, '']
            if 'id' in a:
                self._hid = a['id']
            else:
                self._hid = None
            return
        if n == 'prolog':
            return
        if n in ('title', 'author', 'date', 'docnum'):
            self.where = n
            if n == 'date' and False:
                pass
            return
        if n in ('eprolog', 'frontm', 'body', 'backm', 'userdoc', 'docprof'):
            self.where = None
            return
        if n == 'euserdoc':
            self.close_lists()
            return
        if n == 'tipage':
            self.where = None
            if self.title:
                self.req('.TL')
                self.out.append(esc(' '.join(self.title)))
            for au in self.author:
                self.req('.AU')
                self.out.append(esc(au))
            for x in self.docnum + self.date:
                self.req('.AI')
                self.out.append(esc(x))
            return
        if n == 'toc':
            self.toc = True
            return
        if n == 'appendix':
            return
        if n in ('p', 'lp', 'pc'):
            if self.lists or self.dl:
                self.req('.sp 0.5')
            else:
                self.req('.PP' if n == 'p' else '.LP')
            return
        if n == 'note':
            self.req('.LP')
            self.out.append('\\fBNote:\\fP')
            return
        if n in ('ol', 'ul', 'sl'):
            if self.lists:
                self.req('.RS')
            self.lists.append([n, 0])
            return
        if n in ('eol', 'eul', 'esl'):
            if self.lists:
                self.lists.pop()
                if self.lists:
                    self.req('.RE')
            self.req('.LP')
            return
        if n == 'li':
            if not self.lists:
                self.lists.append(['ul', 0])
            k = self.lists[-1]
            k[1] += 1
            if k[0] == 'ol':
                self.req('.IP "%d." 4' % k[1])
            elif k[0] == 'ul':
                self.req('.IP \\(bu 3' if len(self.lists) % 2 else '.IP \\- 3')
            else:
                self.req('.IP "" 3')
            return
        if n in ('dl', 'gl'):
            w = a.get('tsize', '12')
            w = re.sub(r'[^0-9.]', '', w) or '12'
            self.dl.append(w)
            self.req('.LP')
            return
        if n in ('edl', 'egl'):
            if self.dl:
                self.dl.pop()
            self.req('.LP')
            return
        if n in ('dthd', 'dt', 'gt'):
            self.dtterm = ''
            self._dthead = n == 'dthd'
            return
        if n in ('ddhd', 'dd', 'gd'):
            term = (self.dtterm or '').strip()
            self.dtterm = None
            w = self.dl[-1] if self.dl else '12'
            f = '\\fB' if getattr(self, '_dthead', False) else ''
            self.req('.IP "%s%s%s" %sn' % (f, term.replace('"', "''"),
                                            '\\fP' if f else '', w))
            if len(term) > int(float(w)):
                self.req('.br')
            return
        if n in ('xmp', 'lines'):
            self.verbatim += 1
            self.req('.DS L' if n == 'xmp' else '.nf')
            if n == 'xmp':
                self.req('.ft CW')
            self._vb = n
            return
        if n in ('exmp', 'elines'):
            if n == 'exmp':
                self.req('.ft R')
                self.req('.DE')
            else:
                self.req('.fi')
            self.verbatim = max(0, self.verbatim - 1)
            return
        if n == 'fig':
            self.req('.DS C')
            return
        if n == 'figcap':
            self.fig += 1
            self.req('.sp 0.5')
            self.out.append('\\fBFigure %d.\\fP' % self.fig)
            return
        if n == 'efig':
            self.req('.DE')
            return
        if n == 'table':
            cols = a.get('cols', '').split()
            self.table = {'cols': len(cols) or 0, 'rows': [], 'cur': None, 'widths': cols,
                          'head': set(), 'inhead': False}
            return
        if n == 'thd':
            self.table and self.table.update(inhead=True)
            return
        if n == 'ethd':
            self.table and self.table.update(inhead=False)
            return
        if n == 'row':
            if self.table is not None:
                self.table['cur'] = []
                self.table['rows'].append(self.table['cur'])
                if self.table['inhead']:
                    self.table['head'].add(len(self.table['rows']) - 1)
            return
        if n == 'c':
            if self.table is not None and self.table['cur'] is not None:
                self.table['cur'].append('')
            return
        if n == 'etable':
            self.emit_table()
            return
        if n == 'fn':
            self.out.append('\\**')
            self.req('.FS')
            return
        if n == 'efn':
            self.req('.FE')
            return
        if n in ('abstract', 'preface'):
            self.req('.SH')
            self.out.append(n.capitalize())
            self.req('.PP')
            return
        if n in ('eabstract', 'epreface', 'address', 'eaddress'):
            self.req('.LP')
            return
        if n == 'aline':
            self.req('.br')
            return

    def inline(self, name, a):
        n = name.lower()
        if n.startswith('hp'):
            return HP.get(n[2:], '\\fB')
        if n.startswith('ehp'):
            return '\\fP'
        if n == 'q':
            return '\\(lq'
        if n == 'eq':
            return '\\(rq'
        if n in ('cit', 'xph'):
            return '\\fI' if n == 'cit' else '\\f[CR]'
        if n in ('ecit', 'exph'):
            return '\\fP'
        if n in ('hdref', 'figref'):
            return self.ids.get(a.get('refid', '').lower(), a.get('refid', ''))
        if n == 'nameit':
            if 'symbol' in a:
                self.names[a['symbol'].lower()] = a.get('text', '')
            return ''
        return ''

    def emit_table(self):
        t = self.table
        self.table = None
        rows = [r for r in t['rows'] if r]
        if not rows:
            return
        nc = max(t['cols'], max(len(r) for r in rows))
        self.req('.TS')
        self.out.append('allbox tab(\t);')
        fmts = []
        for i, r in enumerate(rows):
            fmts.append(' '.join(['lb' if i in t['head'] else 'l'] * nc))
        # tbl: one format line per distinct row, last one with a period
        uniq = []
        for f in fmts:
            uniq.append(f)
        self.out.extend(uniq[:-1])
        self.out.append(uniq[-1] + '.')
        for r in rows:
            cells = [esc(c) for c in r] + [''] * (nc - len(r))
            self.out.append('\t'.join('T{\n%s\nT}' % c if len(c) > 30 else c
                                      for c in cells))
        self.req('.TE')

    def close_lists(self):
        while self.lists:
            self.lists.pop()
            if self.lists:
                self.out.append('.RE')
        self.dl = []

    # ---- input ----------------------------------------------------------------
    def load(self, path, depth=0):
        try:
            lines = open(path, encoding='latin-1').read().split('\n')
        except IOError:
            self.out.append('.\\" bookie: cannot read %s' % path)
            return
        for line in lines:
            self.line(line.rstrip('\r'), os.path.dirname(path), depth)

    def find(self, name, here):
        cands = [name, name.lower(), name.upper()]
        for d in [here] + self.inc:
            for c in cands:
                for ext in ('', '.script', '.SCRIPT', '.bkm', '.BKM'):
                    p = os.path.join(d, c + ext)
                    if os.path.exists(p):
                        return p
        return None

    def line(self, line, here, depth):
        if line.startswith('.*'):
            return
        if self.verbatim and not re.match(r'\s*:e(xmp|lines)\.', line, re.I):
            self.out.append(('\\&' if line[:1] in ".'" else '') +
                            self.symbols(esc(line)) if line else '')
            return
        m = re.match(r'\.([a-zA-Z]{2})\b\s*(.*)$', line)
        if m:
            cw, arg = m.group(1).lower(), m.group(2).strip()
            if cw == 'sp':
                self.req('.sp %s' % (arg or '1'))
            elif cw == 'br':
                self.req('.br')
            elif cw == 'pa':
                self.req('.bp')
            elif cw == 'ce':
                self.req('.ce %s' % (arg or '1'))
            elif cw == 'im' and depth < 8:
                p = self.find(arg.split()[0], here) if arg else None
                if p:
                    self.load(p, depth + 1)
                else:
                    self.req('.\\" bookie: .im %s not found' % arg)
            return
        pos = 0
        buf = ''
        line = line
        while True:
            m = TAG.search(line, pos)
            if not m:
                buf += self.symbols(esc(line[pos:]))
                break
            name = m.group(1).lower()
            if name not in BLOCK and name not in INLINE:
                buf += self.symbols(esc(line[pos:m.end()]))
                pos = m.end()
                continue
            buf += self.symbols(esc(line[pos:m.start()]))
            a = attrs(m.group(2))
            if name in INLINE:
                buf += self.inline(name, a)
            else:
                self.text(buf)
                buf = ''
                self.tag(name, a)
                if name in ('h0', 'h1', 'h2', 'h3', 'h4', 'h5', 'h6'):
                    pass
            pos = m.end()
        if buf:
            self.text(buf)
        if hasattr(self, 'endline'):
            self.endline()
        # a heading is the rest of its line
        if self.heading is not None:
            txt = self.heading[1]
            if getattr(self, '_hid', None):
                self.ids[self._hid.lower()] = txt
            self.flush_heading()

    def result(self):
        self.flush_heading()
        self.close_lists()
        head = ['.\\" made by bookie (VM/370plus), BookMaster markup -> ms',
                '.nr PS 11', '.nr VS 13', '.nr LL 6.5i', '.nr HM 1i', '.nr FM 1i',
                '.ds CH', '.ds CF - \\\\n% -']
        tail = ['.TC'] if self.toc else []
        return '\n'.join(head + self.out + tail) + '\n'



class Vmplus(Bookie):
    """the VM/370plus manual style: emits the BK-* macros of vmplus.tmac"""
    FIELDS = ('title', 'author', 'date', 'docnum', 'prodname', 'subtitle',
              'version', 'based', 'filenum', 'stitle')

    def __init__(self, incdirs, toc=None):
        Bookie.__init__(self, incdirs)
        for f in self.FIELDS:
            setattr(self, f, [])
        self.tocent = toc
        self.inbody = False
        self.caption = None
        self.lv = 0

    @staticmethod
    def q(s):
        return '"%s"' % s.replace('"', '\\(dq')

    def text(self, t):
        if self.caption is not None:
            self.caption += (' ' if self.caption else '') + t.strip()
            return
        Bookie.text(self, t)

    def flush_heading(self):
        if self.heading is None:
            return
        lvl, txt = self.heading
        self.heading = None
        if not self.inbody:
            self.out.append('.BK-FH%s %s' % ('' if lvl == 1 else '2', self.q(txt)))
        else:
            self.out.append('.BK-H %d %s' % (lvl, self.q(txt)))

    def endline(self):
        if self.caption is not None:
            c, self.caption = self.caption, None
            self.req('.BK-FIGCAP %s' % c)

    def ds(self, name, val):
        self.out.append('.ds %s %s' % (name, val))

    def tag(self, name, a):
        n = name.lower()
        if n in self.FIELDS:
            self.where = n
            return
        if n in ('eprolog', 'frontm', 'userdoc', 'docprof', 'euserdoc'):
            self.where = None
            if n == 'euserdoc':
                self.req('.BK-ELIST-ALL')
            return
        if n == 'tipage':
            self.where = None
            j = lambda f: ' '.join(getattr(self, f))
            self.titled = True
            self.ds('bk-title', j('stitle') or j('title'))
            self.ds('bk-docnum', j('docnum'))
            self.ds('bk-product', j('prodname'))
            self.ds('bk-stitle', j('subtitle'))
            self.ds('bk-version', j('version') or j('date'))
            self.ds('bk-based', j('based') or j('author'))
            self.ds('bk-pubno', ('Publication ' + j('docnum')) if self.docnum else '')
            self.ds('bk-fileno', ('File No. ' + j('filenum')) if self.filenum else '')
            self.out.append('.ds bk-title %s' % (j('title')))
            self.req('.BK-TITLE')
            self.ds('bk-title', j('stitle') or j('title'))
            return
        if n == 'edition':
            self.where = None
            self.req('.BK-EDITION %s' % esc(a.get('title', 'Edition Notice')))
            return
        if n == 'eedition':
            self.req('.BK-EEDITION')
            return
        if n == 'toc':
            self.req('.BK-CONTENTS')
            for lvl, page, txt in (self.tocent or []):
                if lvl <= 2:
                    self.out.append('.BK-TOCE %d %s %s' % (lvl, page, self.q(txt)))
            self.req('.BK-ECONTENTS')
            return
        if n == 'body':
            self.where = None
            self.inbody = True
            self.req('.BK-BODY')
            return
        if n == 'appendix':
            self.req('.BK-APPENDIX')
            return
        if n == 'backm':
            return
        if n in ('h0', 'h1', 'h2', 'h3', 'h4', 'h5', 'h6'):
            self.req('.BK-ELIST-ALL')
            self.lv = 0
            self.heading = [max(1, int(n[1])), '']
            self._hid = a.get('id')
            return
        if n in ('p', 'lp', 'pc'):
            self.req('.BK-P')
            return
        if n in ('note', 'attention'):
            self.req('.BK-NOTE %s' % ('Note:' if n == 'note' else 'Attention:'))
            return
        if n in ('ol', 'ul', 'sl'):
            self.lv += 1
            self.req('.BK-LIST %s' % n)
            return
        if n in ('eol', 'eul', 'esl', 'edl', 'egl'):
            if self.lv:
                self.lv -= 1
            self.req('.BK-ELIST')
            return
        if n == 'li':
            if not self.lv:
                self.lv = 1
                self.req('.BK-LIST ul')
            self.req('.BK-LI')
            return
        if n in ('dl', 'gl'):
            w = re.sub(r'[^0-9]', '', a.get('tsize', '12')) or '12'
            self.lv += 1
            self.req('.BK-LIST dl %s' % w)
            return
        if n in ('dthd', 'dt', 'gt'):
            self.dtterm = ''
            return
        if n in ('ddhd', 'dd', 'gd'):
            term = (self.dtterm or '').strip()
            self.dtterm = None
            self.req('.BK-DT %s' % self.q(term))
            return
        if n == 'xmp':
            self.verbatim += 1
            self.req('.BK-XMP')
            return
        if n == 'exmp':
            self.verbatim = max(0, self.verbatim - 1)
            self.req('.BK-EXMP')
            return
        if n == 'lines':
            self.verbatim += 1
            self.req('.BK-LINES')
            return
        if n == 'elines':
            self.verbatim = max(0, self.verbatim - 1)
            self.req('.BK-ELINES')
            return
        if n == 'fig':
            self.req('.br')
            return
        if n == 'figcap':
            self.caption = ''
            return
        if n == 'efig':
            return
        if n in ('abstract', 'preface'):
            self.req('.BK-FH %s' % n.capitalize())
            return
        if n in ('eabstract', 'epreface', 'address', 'eaddress', 'aline'):
            self.req('.br')
            return
        if n in ('fn', 'efn'):
            self.out.append('(' if n == 'fn' else ')')
            return
        Bookie.tag(self, name, a)

    def emit_table(self):
        t = self.table
        self.table = None
        rows = [r for r in t['rows'] if r]
        if not rows:
            return
        nc = max(t['cols'], max(len(r) for r in rows))
        self.req('.BK-TS')
        self.out.append('.TS H')
        self.out.append('allbox tab(\t);')
        ws = [float(w) for w in t.get('widths', []) if re.match(r'^[0-9.]+$', w)]
        if len(ws) == nc and sum(ws) > 0:
            wcol = ['w(%.2fc)' % (14.6 * w / sum(ws)) for w in ws]
        else:
            wcol = [''] * nc
        fm = []
        for i, r in enumerate(rows):
            fm.append(' '.join(['l' + ('fHB' if i in t['head'] else '') + wcol[k]
                                for k in range(nc)]))
        self.out.extend(fm[:-1])
        self.out.append(fm[-1] + '.')
        nh = max([i + 1 for i in t['head']] or [0])
        if nh == 0:
            self.out.append('.TH')
        for i, r in enumerate(rows):
            cells = list(r) + [''] * (nc - len(r))
            self.out.append('\t'.join('T{\n%s\nT}' % c for c in cells))
            if i == nh - 1:
                self.out.append('.TH')
        self.out.append('.TE')
        self.req('.BK-TE')

    def result(self):
        self.flush_heading()
        head = ['.\\" made by bookie (VM/370plus): BookMaster markup -> vmplus.tmac']
        if getattr(self, 'titled', False):
            head.append('.nr bk-front 1')
        return '\n'.join(head + self.out) + '\n'

def build(files, inc, style, toc=None, stdin_lines=None):
    b = Vmplus(inc, toc) if style == 'vmplus' else Bookie(inc)
    if files:
        for f in files:
            b.load(f)
    else:
        for line in stdin_lines:
            b.line(line.rstrip('\r'), '.', 0)
    return b.result()


def groff(roff, mode, style):
    import subprocess
    here = os.path.dirname(os.path.abspath(__file__))
    if style == 'vmplus':
        dev = {'text': ['-Tascii', '-P-cbou'], 'ps': ['-Tps', '-dpaper=a4', '-P-pa4']}[mode]
        cmd = ['groff', '-M' + here, '-mvmplus', '-t', '-Wfont', '-Wbreak'] + dev
    else:
        dev = {'text': ['-Tascii', '-P-cbou'], 'ps': ['-Tps']}[mode]
        mdir = os.environ.get('BOOKIE_TMAC', '/usr/local/share/groff/tmac123')
        cmd = ['groff', '-M' + mdir, '-ms', '-t', '-Wfont'] + dev
    p = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    o, e = p.communicate(roff.encode('latin-1'))
    toc, msgs = [], []
    for l in e.decode('latin-1').split('\n'):
        m = re.match(r'BKTOC (\d+) (\S+) (.*)$', l)
        if m:
            toc.append((int(m.group(1)), m.group(2), m.group(3)))
        elif l.strip():
            msgs.append(l)
    return o, toc, msgs, p.returncode


def main(argv):
    out = None; inc = []; mode = None; files = []; style = 'vmplus'
    i = 0
    while i < len(argv):
        a = argv[i]
        if a == '-o':
            out = argv[i + 1]; i += 1
        elif a == '-I':
            inc.append(argv[i + 1]); i += 1
        elif a == '--groff':
            mode = argv[i + 1]; i += 1
        elif a == '--style':
            style = argv[i + 1]; i += 1
        else:
            files.append(a)
        i += 1
    lines = None if files else sys.stdin.read().split('\n')
    if not mode:
        roff = build(files, inc, style, None, lines)
        (open(out, 'w', encoding='latin-1') if out else sys.stdout).write(roff)
        return 0
    # the contents need the page numbers: format until they stand still
    toc = None
    for n in range(4):
        roff = build(files, inc, style, toc, lines)
        o, newtoc, msgs, rc = groff(roff, mode, style)
        if style != 'vmplus' or newtoc == toc:
            break
        toc = newtoc
    for m in msgs:
        sys.stderr.write(m + '\n')
    (open(out, 'wb') if out else sys.stdout.buffer).write(o)
    return rc


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
