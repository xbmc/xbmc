# BSD sockets on Tizen

Browsers give WebAssembly no TCP or UDP sockets. Samsung's Tizen web runtime
does: the Tizen Sockets Extension exposes POSIX-style sockets and synchronous
DNS to WASM pages through `window.tizentvwasm`. Samsung documents it only for
their Emscripten fork (1.39.4, 2021), but the extension is a runtime feature,
so an upstream Emscripten build can use it through a JS library. That is what
`xbmc/platform/wasm/network/tizen_sockets.js` and `TizenSockets.c` do.

## Runtime surface

| Object | Where | Purpose |
|---|---|---|
| `tizentvwasm.SocketsManager` | Workers only | create, connect, bind, listen, accept4, send/recv (+To/From/Msg), poll, select, get/setSockOpt, shutdown, close, getErrorCode |
| `tizentvwasm.SocketsHostBindings` | main thread | raw variants meant as direct wasm imports; not used |
| `tizentvwasm.HostResolverSync` | Workers | `getHostByName(name).getAddrList()` |
| `NetAddress`, `PollFd`, `PollFlags`, `SockFlags`, `MsgFlags`, `ErrorCodes`, `SockOptTimeVal`, `SockOptSoLinger`, `IpMreq`, `Ipv6Mreq` | both | value types |

`tizentvwasm.availableApis` lists `TizenSockets`, `WebRTC` and
`ElementaryMediaStreamSource`. Support starts with 2020 models (Tizen 5.5).
The `internet` privilege in `config.xml` is all that is required.

## Design

- **Runs on the calling pthread.** Emscripten proxies every syscall to the
  browser main thread by default, where `SocketsManager` does not exist. The
  overrides are declared `__proxy: 'none'`; with `PROXY_TO_PTHREAD` Kodi's
  threads are all Workers.
- **Shared fd table.** `TizenSockets.c` owns a table indexed by Emscripten fd
  holding the native socket id, flags (O_NONBLOCK, datagram, IPv6) and the
  SO_RCVTIMEO/SO_SNDTIMEO values. Any thread can use a socket another thread
  created. fds are reserved through a placeholder stream in the main-thread FS
  so they never collide with file descriptors.
- **Always non-blocking natively.** The runtime cannot switch a socket's mode
  after creation, but libcurl toggles O_NONBLOCK with `fcntl`. Native sockets
  are therefore created with `SOCK_NONBLOCK`, and blocking calls wait with
  `SocketsManager.poll` first. `connect` on a blocking socket waits for
  POLLOUT and reads SO_ERROR.
- **errno translation.** The runtime reports Linux errno values; Emscripten's
  libc uses its own numbering (EAGAIN is 6, EINPROGRESS is 26). Every error
  passes through `kodi_sock_errno_from_linux`.
- **poll.** Socket-only sets go to `SocketsManager.poll`. File-only sets go to
  upstream. Mixed sets, which libcurl produces through its wakeup pipe, are
  served in 50 ms slices: sockets are polled with a bounded timeout and files
  are probed through the upstream zero-timeout poll on the main thread.
  `select()` is implemented by musl on top of `poll()`.
- **Fallback.** The upstream implementations are re-registered as
  `kodi_upstream_*` and used whenever `SocketsManager` is absent or an fd is
  not a socket, so desktop browsers behave exactly as before.

`kodi_wasm_has_sockets()` reports the capability to C++; the result is cached
and identical on every thread.

## What Kodi does with it

One binary serves both environments and picks the HTTP backend at runtime:

| | Tizen (sockets) | Other browsers |
|---|---|---|
| `http`, `https` | `CCurlFile` (libcurl) | `CXhrFile` (XMLHttpRequest, `xbmc/filesystem/wasm/`) |
| Scrapers, `CreateHttpClient()` | `CCurlHttpClient` | `CXhrHttpClient` |
| `CCurlFile::GetMimeType` and friends | libcurl | forwarded to `CXhrFile` |

FileFactory makes the choice, so code that goes through `CFile` needs no
changes; code that uses `CCurlFile` directly, and the other libcurl protocols,
still need sockets. The dev proxy (`ENABLE_WASM_DEV_PROXY`) rewrites XHRs only
and does not apply to libcurl traffic.

## Limits

- `poll` cannot mix descriptor types natively; mixed sets cost up to one slice
  of latency.
- Only options in the table in `tizen_sockets.js` are forwarded; others return
  ENOPROTOOPT.
- `SocketsHostBindings` on the main thread is not used, so nothing socket
  related may run on the browser main thread.
- `getaddrinfo` returns one node per address. Emscripten up to 6.0.9 frees
  only the first node in `freeaddrinfo`, so use 6.0.10 or later.
