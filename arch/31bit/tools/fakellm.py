#!/usr/bin/env python3
"""fakellm.py [ADDRESS [PORT]] -- a stand-in for llama.cpp's server in tests:
answers /v1/chat/completions (and Anthropic's /v1/messages) with a canned
reply that quotes the last question and counts the messages it got."""
import json, sys
from http.server import BaseHTTPRequestHandler, HTTPServer
class H(BaseHTTPRequestHandler):
    def do_POST(self):
        b = json.loads(self.rfile.read(int(self.headers['Content-Length'])))
        q = b['messages'][-1]['content']
        txt = ('FAKELLM (%s, %d messages): you asked "%s". VM/370 was announced by IBM in 1972; '
               'CP runs virtual machines and CMS is the single-user system that runs in them. '
               'This sentence is long enough to need wrapping on an 80-column terminal.' % (b.get('model'), len(b['messages']), q.split('\n')[0][:60]))
        if self.path.endswith('/messages'):
            r = {'content': [{'type': 'text', 'text': txt}]}
        else:
            r = {'choices': [{'message': {'role': 'assistant', 'content': txt}}]}
        d = json.dumps(r).encode()
        self.send_response(200); self.send_header('Content-Type', 'application/json'); self.send_header('Content-Length', str(len(d))); self.end_headers(); self.wfile.write(d)
        print('asked:', q[:200].replace('\n', ' | '), flush=True)
    def log_message(self, *a): pass
HTTPServer((sys.argv[1] if len(sys.argv) > 1 else '10.1.1.1', int(sys.argv[2]) if len(sys.argv) > 2 else 8080), H).serve_forever()
