#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""
Minimal HTTP server for Kodi WASM builds.

- Serves static files with the COOP/COEP headers required by SharedArrayBuffer.
- Exposes a same-origin streaming proxy at `/proxy?u=<url-encoded>` so the
  browser can reach http(s) servers that don't send CORS headers. Only loopback
  clients may use it unless --allow-lan-proxy is given. Build with
  -DENABLE_WASM_DEV_PROXY=ON so the module routes its cross-origin requests
  through it (tools/wasm/dev_proxy_pre.js).

Usage:
  cd build-wasm
  cp ../tools/wasm/kodi.html .
  python3 ../tools/wasm/serve.py          # http://127.0.0.1:8080
  python3 ../tools/wasm/serve.py 9000
  python3 ../tools/wasm/serve.py 0.0.0.0:8080  # serve media to a Kodi on the LAN

The launcher itself only works from 127.0.0.1 or localhost: a page loaded over
plain http from any other address is not a secure context, so the browser
withholds SharedArrayBuffer and the pthread runtime cannot start. Binding to
0.0.0.0 is for exposing this directory as a media source to another Kodi.
"""

import http.server
import ipaddress
import socketserver
import sys
import urllib.error
import urllib.parse
import urllib.request


class Handler(http.server.SimpleHTTPRequestHandler):
    allow_lan_proxy = False
    extensions_map = {
        **http.server.SimpleHTTPRequestHandler.extensions_map,
        ".wasm": "application/wasm",
        ".js": "application/javascript",
    }

    def end_headers(self):
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        # CORS-enabled so a Kodi running elsewhere (a TV on the LAN) can use
        # this server's directory listings as a media source.
        self.send_header("Cross-Origin-Resource-Policy", "cross-origin")
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, HEAD, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "*")
        self.send_header("Access-Control-Expose-Headers", "*")
        self.send_header("Cache-Control", "no-cache")
        super().end_headers()

    def do_OPTIONS(self):
        self.send_response(204)
        self.end_headers()

    def do_GET(self):
        if self.path.startswith("/proxy?"):
            self._proxy(body=True)
        else:
            super().do_GET()

    def do_HEAD(self):
        if self.path.startswith("/proxy?"):
            self._proxy(body=False)
        else:
            super().do_HEAD()

    def _client_is_loopback(self) -> bool:
        ip = ipaddress.ip_address(self.client_address[0])
        if isinstance(ip, ipaddress.IPv6Address) and ip.ipv4_mapped:
            ip = ip.ipv4_mapped
        return ip.is_loopback

    def _proxy(self, body: bool):
        if not self.allow_lan_proxy and not self._client_is_loopback():
            self.send_error(403, "proxy is loopback-only; start with --allow-lan-proxy")
            return
        url = urllib.parse.parse_qs(self.path.split("?", 1)[1]).get("u", [""])[0]
        if not url.startswith(("http://", "https://")):
            self.send_error(400, "bad ?u=")
            return

        req = urllib.request.Request(url, method=self.command)
        # Forward a few request headers so the upstream sees a real client.
        # Notably, download.blender.org returns 403 to the default
        # "Python-urllib/3.x" agent.
        for name in ("Range", "User-Agent", "Referer"):
            value = self.headers.get(name)
            if value is not None:
                req.add_header(name, value)
        req.add_header("Accept-Encoding", "identity")
        if req.get_header("User-agent") is None:
            req.add_header("User-Agent", "Mozilla/5.0 (Kodi-WASM proxy)")

        try:
            upstream = urllib.request.urlopen(req, timeout=15)
        except urllib.error.HTTPError as exc:
            upstream = exc
        except Exception as exc:
            self.send_error(502, f"upstream: {exc}")
            return

        with upstream:
            self.send_response(upstream.status)
            for name in ("Content-Type", "Content-Length", "Content-Range",
                         "Accept-Ranges", "Last-Modified", "ETag"):
                value = upstream.headers.get(name)
                if value is not None:
                    self.send_header(name, value)
            self.end_headers()
            if body and self.command != "HEAD":
                try:
                    while chunk := upstream.read(64 * 1024):
                        self.wfile.write(chunk)
                except (BrokenPipeError, ConnectionResetError):
                    pass


class ThreadingServer(socketserver.ThreadingMixIn, http.server.HTTPServer):
    daemon_threads = True
    allow_reuse_address = True


if __name__ == "__main__":
    args = sys.argv[1:]
    if "--allow-lan-proxy" in args:
        args.remove("--allow-lan-proxy")
        Handler.allow_lan_proxy = True
    bind = "127.0.0.1"
    port_arg = args[0] if args else "8080"
    if ":" in port_arg:
        bind, port_text = port_arg.rsplit(":", 1)
        port = int(port_text)
    else:
        port = int(port_arg)
    print(f"Serving http://{bind}:{port}/kodi.html  (Ctrl-C to stop)")
    print(f"Proxy:   http://{bind}:{port}/proxy?u=<url-encoded>")
    ThreadingServer((bind, port), Handler).serve_forever()
