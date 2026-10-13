#!/usr/bin/env python3
"""minicurl -- the part of curl that RXPIP (and the setup) use, for the Debian
guest of VM/370plus, which has no curl (VM/370plus 9.26).  Python 3.2.

  curl [-s] [-f] [-o FILE] [-X METHOD] [-H 'Name: value' | -H @FILE]...
       [--data-binary @FILE|DATA] [-d DATA] [-w '%{http_code}'] URL

Exit codes as curl: 0 ok, 22 HTTP error with -f, 6/7 cannot connect.
"""
import sys
import urllib.request
import urllib.error


def main(argv):
    out = None; method = None; headers = []; data = None
    fail = False; wfmt = None; url = None
    i = 0
    while i < len(argv):
        a = argv[i]
        def val():
            return argv[i + 1]
        if a in ('-s', '--silent', '-S', '-L'):
            pass
        elif a in ('-f', '--fail'):
            fail = True
        elif a in ('-o', '--output'):
            out = val(); i += 1
        elif a in ('-X', '--request'):
            method = val(); i += 1
        elif a in ('-H', '--header'):
            h = val(); i += 1
            if h.startswith('@'):
                for line in open(h[1:], encoding='latin-1'):
                    if ':' in line:
                        headers.append(line.rstrip('\r\n'))
            else:
                headers.append(h)
        elif a in ('--data-binary', '-d', '--data'):
            d = val(); i += 1
            if d.startswith('@'):
                data = open(d[1:], 'rb').read()
            else:
                data = d.encode('latin-1')
        elif a in ('-w', '--write-out'):
            wfmt = val(); i += 1
        elif a.startswith('-'):
            sys.stderr.write('minicurl: option %s not supported\n' % a)
            return 2
        else:
            url = a
        i += 1
    if not url:
        sys.stderr.write('minicurl: no URL\n')
        return 2
    req = urllib.request.Request(url, data=data)
    if method:
        req.get_method = lambda: method
    for h in headers:
        k, v = h.split(':', 1)
        req.add_header(k.strip(), v.strip())
    if data is not None and not any(h.lower().startswith('content-type') for h in headers):
        req.add_header('Content-Type', 'application/x-www-form-urlencoded')
    code = 0
    try:
        r = urllib.request.urlopen(req, timeout=120)
        code = r.getcode(); body = r.read()
    except urllib.error.HTTPError as e:
        code = e.code; body = e.read()
        if fail:
            if wfmt:
                sys.stdout.write(wfmt.replace('%{http_code}', str(code)).replace('\\n', '\n'))
            return 22
    except (urllib.error.URLError, OSError) as e:
        sys.stderr.write('minicurl: %s\n' % e)
        return 7
    if out:
        open(out, 'wb').write(body)
    else:
        sys.stdout.buffer.write(body)
    if wfmt:
        sys.stdout.write(wfmt.replace('%{http_code}', str(code)).replace('\\n', '\n'))
    sys.stdout.flush()
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
