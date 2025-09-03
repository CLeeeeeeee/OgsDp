#!/usr/bin/env python3

import json
import sys
from http.server import HTTPServer, BaseHTTPRequestHandler
import subprocess

DMF_URL = 'http://127.0.0.21:7777/ndmf-gnb-sync/v1/gnb-sync'


class RANProxyHandler(BaseHTTPRequestHandler):
    def _send(self, code=200, body=None):
        data = (body if isinstance(body, (bytes, bytearray)) else json.dumps(body or {"ok": True}).encode())
        self.send_response(code)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Content-Length', str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def do_POST(self):
        try:
            length = int(self.headers.get('Content-Length', '0'))
            raw = self.rfile.read(length) if length > 0 else b'{}'
            payload = json.loads(raw.decode() or '{}')
        except Exception as e:
            self._send(400, {"ok": False, "error": f"bad json: {e}"})
            return

        # Simulate RAN parsing, then forward to DMF via HTTP/2 using curl
        try:
            proc = subprocess.run([
                'curl', '-s', '--http2-prior-knowledge',
                '-H', 'content-type: application/json',
                '-d', json.dumps(payload), DMF_URL
            ], capture_output=True, text=True)
            # We don't strictly require 200 from DMF for test flow
            self._send(200, {"ok": True, "dmf_rc": proc.returncode, "dmf_stdout": proc.stdout[-200:], "dmf_stderr": proc.stderr[-200:]})
        except Exception as e:
            self._send(500, {"ok": False, "error": str(e)})


def main():
    host = '127.0.0.30'
    port = 7788
    if len(sys.argv) >= 2:
        host = sys.argv[1]
    if len(sys.argv) >= 3:
        port = int(sys.argv[2])
    httpd = HTTPServer((host, port), RANProxyHandler)
    print(f"RAN proxy listening on http://{host}:{port}")
    httpd.serve_forever()


if __name__ == '__main__':
    main()


