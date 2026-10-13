# HTTPD -- a web server on CMS (milestone 9.5)

`HTTPD` serves the files of a CMS disk over HTTP, through NICOF's sockets.
The port is a port of the PC, opened by NICOF's Java proxy.

    NCFACC
    HTTPD -p 8090                  the A disk on http://<PC>:8090/
    HTTPD -p 8090 -d B -n 100      the B disk, end after 100 requests
    HTTPD -p 8090 -k secret        GET /.stop/secret ends it
    HTTPD ... -v                   log every send

What it serves:

- `/` serves `INDEX HTML` if the disk has one, else a linked list of the
  files on the disk.
- `/fn.ft` serves the file `fn ft` on the disk; `/fn.ft.fm` serves it from
  disk fm.
- **Text types** go out as ASCII lines with LF. **Binary types** (GIF, PNG,
  JPG, ICO, PDF, ZIP, BIN) go out as the record bytes.
- `text/html`, `text/css`, `text/javascript` and the image types get their
  own `Content-Type`; everything else is `text/plain`.
- A missing file gets a 404 page. Only GET is supported; anything else
  gets 501.

It serves one request at a time, using HTTP/1.0 with `Connection: close`.
The HX command stops it.

## Verified (http3, 13 October)

All of these went from curl on the PC to HTTPD in CMSUSER (16M, IPL CMS):

- `GET /INDEX.HTML` returns 200 with the page.
- `GET /` returns the index page.
- `GET /HTTPD.C` returns the source as text.
- `GET /NOSUCH.FILE` returns 404.
- `GET /HTTPD.TEXT` returns 200.

It is built with CE's GCC against NICOF's client rebuilt with the same
compiler (`../cegcc`).
