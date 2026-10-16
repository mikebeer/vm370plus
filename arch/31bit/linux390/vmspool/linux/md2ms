#!/usr/bin/env python3
"""md2ms.py -- VM/370plus 9.42: Markdown for printing.

    md2ms.py [--text [--width N]] [FILE]      (stdin if no FILE)

Default: troff ms with tbl tables, for  groff -ms -t -Tps  (or -Tpdf,
-Tutf8).  --text: plain text laid out in N columns (default 132), for a
line printer or a terminal -- the same rendering MDPRINT EXEC does on CMS.

The Markdown understood (CommonMark's common core and GitHub tables):
ATX and setext headings, paragraphs, hard breaks (two trailing spaces or a
backslash), *emphasis*, **strong**, `code`, [links](url), ![images](url),
bullet and numbered lists (nested by indentation), > block quotes, fenced
(``` or ~~~) and indented code, horizontal rules, | tables | with an
alignment row, backslash escapes, and HTML comments (dropped).

Written for Python 3.2 (the Debian 7 guest) as well as current ones.
"""
import re
import sys

# ------------------------------------------------------------------ blocks
# The parser turns the text into blocks:
#   ('h', level, text)  ('p', [lines])  ('code', [lines])  ('quote', [blocks])
#   ('list', ordered, start, [[blocks], ...])  ('hr',)  ('table', aligns, rows)

FENCE = re.compile(r'^(\s{0,3})(```+|~~~+)\s*(\S*)')
ATX = re.compile(r'^\s{0,3}(#{1,6})(?:\s+(.*?))?\s*#*\s*$')
class _Rule(object):
    """a thematic break: three or more of one of - * _, spaces between
    (not a regex: Python 3.2's re rejects (\s*\1){2,}, 'nothing to repeat')"""
    def match(self, line):
        if len(line) - len(line.lstrip(' ')) > 3:
            return False
        t = ''.join(line.split())
        return len(t) >= 3 and t[0] in '-*_' and t == t[0] * len(t)


HR = _Rule()
BULLET = re.compile(r'^(\s*)([-*+])\s+(.*)$')
ORDERED = re.compile(r'^(\s*)(\d{1,9})[.)]\s+(.*)$')
TABLESEP = re.compile(r'^\s*\|?\s*:?-+:?\s*(\|\s*:?-+:?\s*)*\|?\s*$')


def split_row(line):
    s = line.strip()
    if s.startswith('|'):
        s = s[1:]
    if s.endswith('|') and not s.endswith('\\|'):
        s = s[:-1]
    cells, cur, i = [], '', 0
    while i < len(s):
        c = s[i]
        if c == '\\' and i + 1 < len(s) and s[i + 1] == '|':
            cur += '|'
            i += 2
            continue
        if c == '|':
            cells.append(cur.strip())
            cur = ''
        else:
            cur += c
        i += 1
    cells.append(cur.strip())
    return cells


def parse(lines):
    blocks = []
    i, n = 0, len(lines)
    para = []

    def flush():
        if para:
            blocks.append(('p', list(para)))
            del para[:]

    while i < n:
        line = lines[i].rstrip('\n').replace('\t', '    ')
        stripped = line.strip()
        # HTML comments
        if stripped.startswith('<!--'):
            flush()
            while i < n and '-->' not in lines[i]:
                i += 1
            i += 1
            continue
        m = FENCE.match(line)
        if m:
            flush()
            mark = m.group(2)
            code = []
            i += 1
            while i < n and not lines[i].strip().startswith(mark[0] * len(mark)):
                code.append(lines[i].rstrip('\n').replace('\t', '    '))
                i += 1
            i += 1
            blocks.append(('code', code))
            continue
        if not stripped:
            flush()
            i += 1
            continue
        if line.startswith('    ') and not para:
            code = []
            while i < n and (lines[i].startswith('    ') or not lines[i].strip()):
                code.append(lines[i].rstrip('\n')[4:])
                i += 1
            while code and not code[-1].strip():
                code.pop()
            blocks.append(('code', code))
            continue
        m = ATX.match(line)
        if m:
            flush()
            blocks.append(('h', len(m.group(1)), m.group(2) or ''))
            i += 1
            continue
        if para and re.match(r'^\s{0,3}=+\s*$', line):
            text = ' '.join(x.strip() for x in para)
            del para[:]
            blocks.append(('h', 1, text))
            i += 1
            continue
        if para and re.match(r'^\s{0,3}-+\s*$', line):
            text = ' '.join(x.strip() for x in para)
            del para[:]
            blocks.append(('h', 2, text))
            i += 1
            continue
        if HR.match(line):
            flush()
            blocks.append(('hr',))
            i += 1
            continue
        if stripped.startswith('>'):
            flush()
            inner = []
            while i < n and lines[i].strip().startswith('>'):
                t = lines[i].strip()[1:]
                if t.startswith(' '):
                    t = t[1:]
                inner.append(t)
                i += 1
            blocks.append(('quote', parse(inner)))
            continue
        if '|' in line and i + 1 < n and TABLESEP.match(lines[i + 1]) and '-' in lines[i + 1]:
            flush()
            head = split_row(line)
            aligns = []
            for c in split_row(lines[i + 1]):
                c = c.strip()
                if c.startswith(':') and c.endswith(':'):
                    aligns.append('c')
                elif c.endswith(':'):
                    aligns.append('r')
                else:
                    aligns.append('l')
            rows = [head]
            i += 2
            while i < n and '|' in lines[i] and lines[i].strip():
                rows.append(split_row(lines[i]))
                i += 1
            w = max(len(r) for r in rows)
            aligns += ['l'] * (w - len(aligns))
            rows = [r + [''] * (w - len(r)) for r in rows]
            blocks.append(('table', aligns[:w], rows))
            continue
        mb, mo = BULLET.match(line), ORDERED.match(line)
        # a list may interrupt a paragraph (a numbered one only from 1),
        # as in CommonMark
        if (mb or mo) and (not para or mb or (mo and mo.group(2) == '1')):
            flush()
            i = parse_list(lines, i, blocks)
            continue
        para.append(line.strip() if not line.endswith('  ') else line.strip() + '  ')
        i += 1
    flush()
    return blocks


def parse_list(lines, i, blocks):
    n = len(lines)
    first = lines[i].replace('\t', '    ')
    m = BULLET.match(first) or ORDERED.match(first)
    ordered = ORDERED.match(first) is not None
    base = len(m.group(1))
    start = int(m.group(2)) if ordered else 1
    items = []
    while i < n:
        line = lines[i].rstrip('\n').replace('\t', '    ')
        m = (ORDERED if ordered else BULLET).match(line)
        if not m or len(m.group(1)) != base:
            break
        body = [m.group(3)]
        cont = base + 2
        i += 1
        while i < n:
            l2 = lines[i].rstrip('\n').replace('\t', '    ')
            if not l2.strip():
                # a blank line: the item goes on if the next line is indented
                if i + 1 < n and (len(lines[i + 1]) - len(lines[i + 1].lstrip())) > base and lines[i + 1].strip():
                    body.append('')
                    i += 1
                    continue
                break
            ind = len(l2) - len(l2.lstrip())
            if ind <= base and (BULLET.match(l2) or ORDERED.match(l2) or ATX.match(l2) or HR.match(l2)):
                break
            if ind <= base and not body[-1].strip():
                break
            body.append(l2[min(ind, cont):] if ind > base else l2.strip())
            i += 1
        items.append(parse(body))
        while i < n and not lines[i].strip():
            i += 1
            if i < n and not ((BULLET if not ordered else ORDERED).match(lines[i].replace('\t', '    ')) or
                              (len(lines[i]) - len(lines[i].lstrip())) > base):
                break
    blocks.append(('list', ordered, start, items))
    return i


# ------------------------------------------------------------------ inline
INLINE = re.compile(
    r'(?P<esc>\\[\\`*_{}\[\]()#+\-.!|>~])'
    r'|(?P<code>`+)(?P<ctext>.+?)(?P=code)'
    r'|!\[(?P<ialt>[^\]]*)\]\((?P<iurl>[^)\s]*)[^)]*\)'
    r'|\[(?P<ltext>[^\]]+)\]\((?P<lurl>[^)\s]*)[^)]*\)'
    r'|<(?P<auto>https?://[^>\s]+)>'
    r'|(?P<strong>\*\*|__)(?P<stext>.+?)(?P=strong)'
    r'|(?P<em>[*_])(?P<etext>[^*_]+?)(?P=em)'
    r'|(?P<br>  $|\\$)')


def inline(text, out):
    """out: an object with plain(s), bold(s), italic(s), code(s), br()"""
    pos = 0
    for m in INLINE.finditer(text):
        if m.start() > pos:
            out.plain(text[pos:m.start()])
        if m.group('esc'):
            out.plain(m.group('esc')[1])
        elif m.group('code'):
            out.code(m.group('ctext').strip())
        elif m.group('ialt') is not None and m.group('iurl') is not None:
            out.plain('[image: %s]' % (m.group('ialt') or m.group('iurl')))
        elif m.group('ltext'):
            inline(m.group('ltext'), out)
            if m.group('lurl') and m.group('lurl') != m.group('ltext'):
                out.plain(' (%s)' % m.group('lurl'))
        elif m.group('auto'):
            out.plain(m.group('auto'))
        elif m.group('strong'):
            out.bold(m.group('stext'))
        elif m.group('em'):
            out.italic(m.group('etext'))
        elif m.group('br') is not None:
            out.br()
        pos = m.end()
    if pos < len(text):
        out.plain(text[pos:])


# ------------------------------------------------------------------ troff ms
def tr_escape(s):
    s = s.replace('\\', '\\e')
    return s


class TrInline(object):
    def __init__(self):
        self.parts = []

    def plain(self, s):
        self.parts.append(tr_escape(s))

    def bold(self, s):
        self.parts.append('\\fB')
        inline(s, self)
        self.parts.append('\\fP')

    def italic(self, s):
        self.parts.append('\\fI')
        inline(s, self)
        self.parts.append('\\fP')

    def code(self, s):
        self.parts.append('\\f(CW' + tr_escape(s) + '\\fP')

    def br(self):
        self.parts.append('\n.br\n')


def tr_text(text):
    t = TrInline()
    inline(text, t)
    s = ''.join(t.parts)
    lines = []
    for l in s.split('\n'):
        if l[:1] in ('.', "'"):
            l = '\\&' + l
        lines.append(l)
    return '\n'.join(lines)


def to_ms(blocks, out, depth=0):
    for b in blocks:
        k = b[0]
        if k == 'h':
            out.append('.SH %d' % min(b[1], 5))
            out.append(tr_text(b[2]))
        elif k == 'p':
            out.append('.PP' if depth == 0 else '.LP')
            out.append(tr_text(' '.join(x.strip() + ('  ' if x.endswith('  ') else '') for x in b[1]).replace('   ', '  ')))
        elif k == 'code':
            out.append('.DS L')
            out.append('.ft CW')
            out.append('.ps -1')
            for l in b[1]:
                l = l.replace('\\', '\\e')
                if l[:1] in ('.', "'"):
                    l = '\\&' + l
                out.append(l if l else '\\&')
            out.append('.ps')
            out.append('.ft')
            out.append('.DE')
        elif k == 'quote':
            out.append('.QS')
            to_ms(b[1], out, depth + 1)
            out.append('.QE')
        elif k == 'list':
            ordered, num = b[1], b[2]
            if depth:
                out.append('.RS')
            for item in b[3]:
                tag = ('%d.' % num) if ordered else '\\(bu'
                out.append('.IP "%s" %d' % (tag, 4 if ordered else 3))
                first = True
                for sub in item:
                    if first and sub[0] == 'p':
                        out.append(tr_text(' '.join(x.strip() for x in sub[1])))
                    else:
                        to_ms([sub], out, depth + 1)
                    first = False
                num += 1
            if depth:
                out.append('.RE')
        elif k == 'hr':
            out.append('.LP')
            out.append("\\l'\\n(.lu'")
        elif k == 'table':
            aligns, rows = b[1], b[2]
            out.append('.TS')
            out.append('center box tab(\x07);')
            out.append(' '.join(a + 'b' for a in aligns))
            out.append(' '.join(aligns) + '.')
            for r, row in enumerate(rows):
                out.append('\x07'.join(('T{\n' + tr_text(c) + '\nT}') if len(c) > 30 else tr_text(c) for c in row))
                if r == 0:
                    out.append('_')
            out.append('.TE')


def render_ms(blocks):
    out = ['.\\" from Markdown by md2ms.py (VM/370plus)',
           '.nr PS 10', '.nr VS 12', '.nr LL 6.5i', '.nr PO 1i', '.nr HM 0.8i', '.nr FM 0.8i',
           '.ds CH', '.ds CF - % -']
    to_ms(blocks, out)
    return '\n'.join(x for x in out if x is not None) + '\n'


# ------------------------------------------------------------------ text
class TxInline(object):
    def __init__(self):
        self.parts = []

    def plain(self, s):
        self.parts.append(s)

    def bold(self, s):
        inline(s, self)

    def italic(self, s):
        inline(s, self)

    def code(self, s):
        self.parts.append(s)

    def br(self):
        self.parts.append('\n')


def tx_text(text):
    t = TxInline()
    inline(text, t)
    return ''.join(t.parts)


def wrap(text, width, first, rest):
    out = []
    for seg in text.split('\n'):
        words = seg.split()
        line = first
        empty = True
        for w in words:
            if not empty and len(line) + 1 + len(w) > width:
                out.append(line)
                line = rest
                empty = True
            line = line + ('' if empty else ' ') + w
            empty = False
        out.append(line)
        first = rest
    return out


def to_text(blocks, width, out, ind=''):
    for b in blocks:
        k = b[0]
        if out and out[-1] != '':
            out.append('')
        if k == 'h':
            t = tx_text(b[2])
            if b[1] == 1:
                t = t.upper()
            out.extend(wrap(t, width, ind, ind))
            if b[1] <= 2:
                out.append(ind + ('=' if b[1] == 1 else '-') * min(len(t), width - len(ind)))
        elif k == 'p':
            t = tx_text(' '.join(x.strip() + ('  ' if x.endswith('  ') else '') for x in b[1]))
            out.extend(wrap(t, width, ind, ind))
        elif k == 'code':
            for l in b[1]:
                out.append((ind + '    ' + l)[:width])
        elif k == 'quote':
            sub = []
            to_text(b[1], width - 2, sub)
            out.extend(ind + '| ' + l if l else ind + '|' for l in sub)
        elif k == 'list':
            num = b[2]
            for item in b[3]:
                tag = ('%d.' % num) if b[1] else '-'
                tag = tag.ljust(4 if b[1] else 2)
                sub = []
                to_text(item, width - len(ind) - len(tag), sub)
                while sub and sub[0] == '':
                    sub.pop(0)
                for j, l in enumerate(sub):
                    out.append(ind + (tag if j == 0 else ' ' * len(tag)) + l if l else '')
                num += 1
        elif k == 'hr':
            out.append(ind + '-' * (width - len(ind)))
        elif k == 'table':
            aligns, rows = b[1], b[2]
            rows = [[tx_text(c) for c in r] for r in rows]
            w = [max(len(r[c]) for r in rows) for c in range(len(aligns))]
            # too wide: cut the widest columns until it fits
            while sum(w) + 3 * len(w) + 1 + len(ind) > width and max(w) > 4:
                j = w.index(max(w))
                w[j] -= 1
            line = ind + '+' + '+'.join('-' * (x + 2) for x in w) + '+'
            out.append(line)
            for r, row in enumerate(rows):
                # a cell longer than its column is wrapped onto more lines
                cols = [wrap(row[c], w[c], '', '') if row[c] else [''] for c in range(len(aligns))]
                for k in range(max(len(x) for x in cols)):
                    cells = []
                    for c, a in enumerate(aligns):
                        t = cols[c][k][:w[c]] if k < len(cols[c]) else ''
                        t = t.center(w[c]) if a == 'c' else t.rjust(w[c]) if a == 'r' else t.ljust(w[c])
                        cells.append(' ' + t + ' ')
                    out.append(ind + '|' + '|'.join(cells) + '|')
                if r == 0:
                    out.append(line.replace('-', '='))
            out.append(line)


def render_text(blocks, width):
    out = []
    to_text(blocks, width, out)
    return '\n'.join(l.rstrip() for l in out) + '\n'


def main(argv):
    width, text, src = 132, False, None
    i = 0
    while i < len(argv):
        a = argv[i]
        if a == '--text':
            text = True
        elif a == '--width':
            i += 1
            width = int(argv[i])
        elif a in ('-h', '--help'):
            print(__doc__)
            return 0
        else:
            src = a
        i += 1
    data = open(src, encoding='utf-8', errors='replace').read() if src else sys.stdin.read()
    blocks = parse(data.split('\n'))
    sys.stdout.write(render_text(blocks, width) if text else render_ms(blocks))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
