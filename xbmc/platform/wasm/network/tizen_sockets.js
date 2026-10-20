/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

// clang-format splits the {{{ }}} macros of the Emscripten JS library.
// clang-format off

// BSD sockets for Emscripten on Samsung Tizen TVs.
//
// The Tizen web runtime exposes TCP/UDP sockets and synchronous DNS to
// WebAssembly through `tizentvwasm.SocketsManager`, which exists in Workers
// only. The socket syscalls below therefore run on the calling pthread, and
// keep per-fd state in the shared table owned by TizenSockets.c so every
// thread sees the same descriptors. Native sockets are always created
// non-blocking; blocking semantics, SO_RCVTIMEO and SO_SNDTIMEO are
// implemented with poll() on top of them.
//
// Without the extension every entry point falls through to the upstream
// Emscripten implementation, re-registered below under `kodi_upstream_*`.

(() => {
  const lib = LibraryManager.library;
  const upstream = {};
  const names = {
    __syscall_socket: 'socket',
    __syscall_bind: 'bind',
    __syscall_connect: 'connect',
    __syscall_listen: 'listen',
    __syscall_accept4: 'accept4',
    __syscall_getsockname: 'getsockname',
    __syscall_getpeername: 'getpeername',
    __syscall_sendto: 'sendto',
    __syscall_recvfrom: 'recvfrom',
    __syscall_sendmsg: 'sendmsg',
    __syscall_recvmsg: 'recvmsg',
    __syscall_getsockopt: 'getsockopt',
    __syscall_setsockopt: 'setsockopt',
    __syscall_shutdown: 'shutdown',
    __syscall_poll: 'poll',
    __syscall_poll_nonblocking: 'poll_nonblocking',
    __syscall_fcntl64: 'fcntl64',
    __syscall_ioctl: 'ioctl',
    fd_close: 'fd_close',
    fd_read: 'fd_read',
    fd_write: 'fd_write',
    getaddrinfo: 'getaddrinfo',
  };
  for (const [name, suffix] of Object.entries(names)) {
    const alias = 'kodi_upstream_' + suffix;
    upstream[alias] = lib[name];
    upstream[alias + '__deps'] = lib[name + '__deps'] || [];
    upstream[alias + '__proxy'] = lib[name + '__proxy'] || 'sync';
    if (lib[name + '__sig']) upstream[alias + '__sig'] = lib[name + '__sig'];
    if (lib[name + '__async'] !== undefined) upstream[alias + '__async'] = lib[name + '__async'];
  }
  addToLibrary(upstream);
})();

addToLibrary({
  $TizenSockets__deps: [
    'kodi_sock_table', 'kodi_sock_max_fd', 'kodi_sock_errno_from_linux',
    '_kodi_sock_fd_reserve', '_kodi_sock_fd_release',
    'kodi_upstream_poll_nonblocking',
    'malloc', 'free', 'freeaddrinfo',
    '$inetPton4', '$inetPton6', '$UTF8ToString',
  ],
  $TizenSockets: {
    ENTRY_SIZE: 16,
    NATIVE: 0,
    FLAGS: 1,
    RCVTIMEO: 2,
    SNDTIMEO: 3,
    FLAG_NONBLOCK: 1,
    FLAG_DGRAM: 2,
    FLAG_INET6: 4,
    // Time slice used when a poll() set mixes sockets and file descriptors.
    MIXED_POLL_SLICE_MS: 50,

    table: 0,
    maxFd: 0,

    init() {
      if (!TizenSockets.table) {
        TizenSockets.table = _kodi_sock_table();
        TizenSockets.maxFd = _kodi_sock_max_fd();
      }
    },

    available() {
      return typeof tizentvwasm !== 'undefined' && !!tizentvwasm.SocketsManager;
    },

    slot(fd, field) {
      return ((TizenSockets.table + fd * TizenSockets.ENTRY_SIZE) >> 2) + field;
    },

    // Native Tizen socket id for an fd, or -1 when the fd is not a socket.
    native(fd) {
      TizenSockets.init();
      if (fd < 0 || fd >= TizenSockets.maxFd) return -1;
      return Atomics.load(HEAP32, TizenSockets.slot(fd, TizenSockets.NATIVE)) - 1;
    },

    flags(fd) {
      return Atomics.load(HEAP32, TizenSockets.slot(fd, TizenSockets.FLAGS));
    },

    setFlags(fd, flags) {
      Atomics.store(HEAP32, TizenSockets.slot(fd, TizenSockets.FLAGS), flags);
    },

    // Configured timeout in ms, or -1 for "wait forever".
    timeout(fd, field) {
      const ms = Atomics.load(HEAP32, TizenSockets.slot(fd, field));
      return ms > 0 ? ms : -1;
    },

    setTimeout(fd, field, ms) {
      Atomics.store(HEAP32, TizenSockets.slot(fd, field), ms);
    },

    register(nativeId, flags) {
      TizenSockets.init();
      const fd = __kodi_sock_fd_reserve();
      if (fd < 0) return fd;
      if (fd >= TizenSockets.maxFd) {
        __kodi_sock_fd_release(fd);
        return -{{{ cDefs.EMFILE }}};
      }
      Atomics.store(HEAP32, TizenSockets.slot(fd, TizenSockets.FLAGS), flags);
      Atomics.store(HEAP32, TizenSockets.slot(fd, TizenSockets.RCVTIMEO), 0);
      Atomics.store(HEAP32, TizenSockets.slot(fd, TizenSockets.SNDTIMEO), 0);
      Atomics.store(HEAP32, TizenSockets.slot(fd, TizenSockets.NATIVE), nativeId + 1);
      return fd;
    },

    unregister(fd) {
      Atomics.store(HEAP32, TizenSockets.slot(fd, TizenSockets.NATIVE), 0);
      __kodi_sock_fd_release(fd);
    },

    // errno of a failed SocketsManager call, translated from the Linux value
    // the runtime reports. It keeps a per-socket error code and repeats it in
    // the exception message as "(nnn)".
    errno(e, nativeId) {
      let code = 0;
      try {
        code = nativeId >= 0 ? tizentvwasm.SocketsManager.getErrorCode(nativeId)
                             : tizentvwasm.SocketsManager.getErrorCode();
      } catch (ignored) {
      }
      if (!code) {
        const m = /\((\d+)\)\s*$/.exec(String(e && e.message || ''));
        if (m) code = parseInt(m[1], 10);
      }
      return code ? _kodi_sock_errno_from_linux(code) : {{{ cDefs.EINVAL }}};
    },

    familyName(af) {
      if (af === {{{ cDefs.AF_INET }}}) return 'af_inet';
      if (af === {{{ cDefs.AF_INET6 }}}) return 'af_inet6';
      return null;
    },

    // sockaddr in wasm memory -> tizentvwasm.NetAddress, or a negative errno.
    readAddr(ptr, len) {
      if (!ptr) return -{{{ cDefs.EFAULT }}};
      const family = {{{ makeGetValue('ptr', C_STRUCTS.sockaddr_in.sin_family, 'u16') }}};
      const port = (HEAPU8[ptr + {{{ C_STRUCTS.sockaddr_in.sin_port }}}] << 8) |
                   HEAPU8[ptr + {{{ C_STRUCTS.sockaddr_in.sin_port }}} + 1];
      let bytes;
      if (family === {{{ cDefs.AF_INET }}}) {
        if (len < {{{ C_STRUCTS.sockaddr_in.__size__ }}}) return -{{{ cDefs.EINVAL }}};
        const off = ptr + {{{ C_STRUCTS.sockaddr_in.sin_addr.s_addr }}};
        bytes = HEAPU8.slice(off, off + 4);
      } else if (family === {{{ cDefs.AF_INET6 }}}) {
        if (len < {{{ C_STRUCTS.sockaddr_in6.__size__ }}}) return -{{{ cDefs.EINVAL }}};
        const off = ptr + {{{ C_STRUCTS.sockaddr_in6.sin6_addr.__in6_union.__s6_addr }}};
        bytes = HEAPU8.slice(off, off + 16);
      } else {
        return -{{{ cDefs.EAFNOSUPPORT }}};
      }
      return new tizentvwasm.NetAddress(TizenSockets.familyName(family), bytes, port);
    },

    // tizentvwasm.NetAddress -> sockaddr in wasm memory, honouring *lenPtr as
    // the buffer capacity and updating it to the full address length.
    writeAddr(netAddr, ptr, lenPtr) {
      if (!ptr || !lenPtr) return;
      const v6 = netAddr.family === 'af_inet6';
      const size = v6 ? {{{ C_STRUCTS.sockaddr_in6.__size__ }}} : {{{ C_STRUCTS.sockaddr_in.__size__ }}};
      const out = new Uint8Array(size);
      const family = v6 ? {{{ cDefs.AF_INET6 }}} : {{{ cDefs.AF_INET }}};
      out[{{{ C_STRUCTS.sockaddr_in.sin_family }}}] = family & 0xff;
      out[{{{ C_STRUCTS.sockaddr_in.sin_family }}} + 1] = family >> 8;
      out[{{{ C_STRUCTS.sockaddr_in.sin_port }}}] = netAddr.port >> 8;
      out[{{{ C_STRUCTS.sockaddr_in.sin_port }}} + 1] = netAddr.port & 0xff;
      out.set(netAddr.bytes, v6 ? {{{ C_STRUCTS.sockaddr_in6.sin6_addr.__in6_union.__s6_addr }}}
                                : {{{ C_STRUCTS.sockaddr_in.sin_addr.s_addr }}});
      const capacity = {{{ makeGetValue('lenPtr', 0, 'i32') }}};
      HEAPU8.set(out.subarray(0, Math.min(capacity, size)), ptr);
      {{{ makeSetValue('lenPtr', 0, 'size', 'i32') }}};
    },

    pollToJs(events) {
      const F = tizentvwasm.PollFlags;
      let js = 0;
      if (events & {{{ cDefs.POLLIN }}}) js |= F.POLLIN;
      if (events & 2 /* POLLPRI */) js |= F.POLLPRI;
      if (events & {{{ cDefs.POLLOUT }}}) js |= F.POLLOUT;
      if (events & 64 /* POLLRDNORM */) js |= F.POLLRDNORM;
      if (events & 128 /* POLLRDBAND */) js |= F.POLLRDBAND;
      if (events & 256 /* POLLWRNORM */) js |= F.POLLWRNORM;
      if (events & 512 /* POLLWRBAND */) js |= F.POLLWRBAND;
      return js;
    },

    pollFromJs(revents) {
      const F = tizentvwasm.PollFlags;
      let c = 0;
      if (revents & F.POLLIN) c |= {{{ cDefs.POLLIN }}};
      if (revents & F.POLLPRI) c |= 2;
      if (revents & F.POLLOUT) c |= {{{ cDefs.POLLOUT }}};
      if (revents & F.POLLERR) c |= {{{ cDefs.POLLERR }}};
      if (revents & F.POLLHUP) c |= {{{ cDefs.POLLHUP }}};
      if (revents & F.POLLNVAL) c |= {{{ cDefs.POLLNVAL }}};
      if (revents & F.POLLRDNORM) c |= 64;
      if (revents & F.POLLRDBAND) c |= 128;
      if (revents & F.POLLWRNORM) c |= 256;
      if (revents & F.POLLWRBAND) c |= 512;
      return c;
    },

    // Block until the native socket reports one of `events`, or the timeout
    // elapses. Returns 1 when ready, 0 on timeout, a negative errno otherwise.
    wait(nativeId, events, timeoutMs) {
      const pfd = new tizentvwasm.PollFd(nativeId, events);
      try {
        return tizentvwasm.SocketsManager.poll([pfd], timeoutMs) > 0 ? 1 : 0;
      } catch (e) {
        return -TizenSockets.errno(e, nativeId);
      }
    },

    waitReadable(fd, nativeId) {
      const F = tizentvwasm.PollFlags;
      return TizenSockets.wait(nativeId, F.POLLIN | F.POLLPRI,
                               TizenSockets.timeout(fd, TizenSockets.RCVTIMEO));
    },

    waitWritable(fd, nativeId) {
      return TizenSockets.wait(nativeId, tizentvwasm.PollFlags.POLLOUT,
                               TizenSockets.timeout(fd, TizenSockets.SNDTIMEO));
    },

    msgFlagsToJs(flags) {
      const F = tizentvwasm.MsgFlags;
      let js = 0;
      if (flags & {{{ cDefs.MSG_PEEK }}}) js |= F.MSG_PEEK;
      if (flags & 1 /* MSG_OOB */) js |= F.MSG_OOB;
      if (flags & 256 /* MSG_WAITALL */) js |= F.MSG_WAITALL;
      if (flags & 16384 /* MSG_NOSIGNAL */) js |= F.MSG_NOSIGNAL;
      return js;
    },

    socket(domain, type, protocol) {
      const family = TizenSockets.familyName(domain);
      if (!family) return -{{{ cDefs.EAFNOSUPPORT }}};
      const baseType = type & ~({{{ cDefs.SOCK_NONBLOCK }}} | {{{ cDefs.SOCK_CLOEXEC }}});
      let typeName;
      if (baseType === {{{ cDefs.SOCK_STREAM }}}) {
        if (protocol && protocol !== {{{ cDefs.IPPROTO_TCP }}}) return -{{{ cDefs.EPROTONOSUPPORT }}};
        typeName = 'sock_stream';
      } else if (baseType === {{{ cDefs.SOCK_DGRAM }}}) {
        if (protocol && protocol !== {{{ cDefs.IPPROTO_UDP }}}) return -{{{ cDefs.EPROTONOSUPPORT }}};
        typeName = 'sock_dgram';
      } else {
        return -{{{ cDefs.EPROTONOSUPPORT }}};
      }
      let nativeId;
      try {
        nativeId = tizentvwasm.SocketsManager.create(family, typeName, tizentvwasm.SockFlags.SOCK_NONBLOCK);
      } catch (e) {
        return -TizenSockets.errno(e, -1);
      }
      let flags = 0;
      if (type & {{{ cDefs.SOCK_NONBLOCK }}}) flags |= TizenSockets.FLAG_NONBLOCK;
      if (baseType === {{{ cDefs.SOCK_DGRAM }}}) flags |= TizenSockets.FLAG_DGRAM;
      if (domain === {{{ cDefs.AF_INET6 }}}) flags |= TizenSockets.FLAG_INET6;
      const fd = TizenSockets.register(nativeId, flags);
      if (fd < 0) {
        try { tizentvwasm.SocketsManager.close(nativeId); } catch (ignored) {}
      }
      return fd;
    },

    close(fd, nativeId) {
      let ret = 0;
      try {
        tizentvwasm.SocketsManager.close(nativeId);
      } catch (e) {
        ret = -TizenSockets.errno(e, nativeId);
      }
      TizenSockets.unregister(fd);
      return ret;
    },

    connect(fd, nativeId, addrPtr, addrLen) {
      const addr = TizenSockets.readAddr(addrPtr, addrLen);
      if (typeof addr === 'number') return addr;
      try {
        tizentvwasm.SocketsManager.connect(nativeId, addr);
        return 0;
      } catch (e) {
        const code = TizenSockets.errno(e, nativeId);
        if (code !== {{{ cDefs.EINPROGRESS }}}) return -code;
      }
      if (TizenSockets.flags(fd) & TizenSockets.FLAG_NONBLOCK) return -{{{ cDefs.EINPROGRESS }}};
      const r = TizenSockets.waitWritable(fd, nativeId);
      if (r < 0) return r;
      if (r === 0) return -{{{ cDefs.ETIMEDOUT }}};
      try {
        const err = tizentvwasm.SocketsManager.getSockOpt(nativeId, 'sol_socket', 'so_error');
        return err ? -_kodi_sock_errno_from_linux(err) : 0;
      } catch (e) {
        return -TizenSockets.errno(e, nativeId);
      }
    },

    bind(nativeId, addrPtr, addrLen) {
      const addr = TizenSockets.readAddr(addrPtr, addrLen);
      if (typeof addr === 'number') return addr;
      try {
        tizentvwasm.SocketsManager.bind(nativeId, addr);
        return 0;
      } catch (e) {
        return -TizenSockets.errno(e, nativeId);
      }
    },

    listen(nativeId, backlog) {
      try {
        tizentvwasm.SocketsManager.listen(nativeId, backlog);
        return 0;
      } catch (e) {
        return -TizenSockets.errno(e, nativeId);
      }
    },

    accept4(fd, nativeId, addrPtr, lenPtr, flags) {
      if (!(TizenSockets.flags(fd) & TizenSockets.FLAG_NONBLOCK)) {
        const r = TizenSockets.waitReadable(fd, nativeId);
        if (r < 0) return r;
        if (r === 0) return -{{{ cDefs.EAGAIN }}};
      }
      let accepted;
      try {
        accepted = tizentvwasm.SocketsManager.accept4(nativeId, tizentvwasm.SockFlags.SOCK_NONBLOCK);
      } catch (e) {
        return -TizenSockets.errno(e, nativeId);
      }
      let newFlags = TizenSockets.flags(fd) & TizenSockets.FLAG_INET6;
      if (flags & {{{ cDefs.SOCK_NONBLOCK }}}) newFlags |= TizenSockets.FLAG_NONBLOCK;
      const newFd = TizenSockets.register(accepted, newFlags);
      if (newFd < 0) {
        try { tizentvwasm.SocketsManager.close(accepted); } catch (ignored) {}
        return newFd;
      }
      if (addrPtr && lenPtr) {
        try {
          TizenSockets.writeAddr(tizentvwasm.SocketsManager.getPeerName(accepted), addrPtr, lenPtr);
        } catch (ignored) {
        }
      }
      return newFd;
    },

    name(nativeId, addrPtr, lenPtr, peer) {
      try {
        const addr = peer ? tizentvwasm.SocketsManager.getPeerName(nativeId)
                          : tizentvwasm.SocketsManager.getSockName(nativeId);
        TizenSockets.writeAddr(addr, addrPtr, lenPtr);
        return 0;
      } catch (e) {
        return -TizenSockets.errno(e, nativeId);
      }
    },

    shutdown(nativeId, how) {
      const name = ['shut_rd', 'shut_wr', 'shut_rdwr'][how];
      if (!name) return -{{{ cDefs.EINVAL }}};
      try {
        tizentvwasm.SocketsManager.shutdown(nativeId, name);
        return 0;
      } catch (e) {
        return -TizenSockets.errno(e, nativeId);
      }
    },

    // `data` is a view over wasm memory that the runtime fills in place.
    recv(fd, nativeId, data, flags, addrPtr, lenPtr) {
      const nonblock = (flags & 64 /* MSG_DONTWAIT */) ||
                       (TizenSockets.flags(fd) & TizenSockets.FLAG_NONBLOCK);
      if (!nonblock) {
        const r = TizenSockets.waitReadable(fd, nativeId);
        if (r < 0) return r;
        if (r === 0) return -{{{ cDefs.EAGAIN }}};
      }
      const jsFlags = TizenSockets.msgFlagsToJs(flags);
      try {
        if (addrPtr && lenPtr) {
          const ret = tizentvwasm.SocketsManager.recvFrom(nativeId, data, jsFlags);
          if (ret.peerAddress) TizenSockets.writeAddr(ret.peerAddress, addrPtr, lenPtr);
          return ret.bytesRead;
        }
        return tizentvwasm.SocketsManager.recv(nativeId, data, jsFlags);
      } catch (e) {
        return -TizenSockets.errno(e, nativeId);
      }
    },

    // A blocking stream socket sends the whole buffer.
    send(fd, nativeId, data, flags, addrPtr, addrLen) {
      const sockFlags = TizenSockets.flags(fd);
      const nonblock = (flags & 64 /* MSG_DONTWAIT */) || (sockFlags & TizenSockets.FLAG_NONBLOCK);
      const stream = !(sockFlags & TizenSockets.FLAG_DGRAM);
      let dest = null;
      if (addrPtr && addrLen) {
        dest = TizenSockets.readAddr(addrPtr, addrLen);
        if (typeof dest === 'number') return dest;
      }
      const jsFlags = TizenSockets.msgFlagsToJs(flags);
      let sent = 0;
      for (;;) {
        let n;
        try {
          const chunk = sent ? data.subarray(sent) : data;
          n = dest ? tizentvwasm.SocketsManager.sendTo(nativeId, chunk, jsFlags, dest)
                   : tizentvwasm.SocketsManager.send(nativeId, chunk, jsFlags);
        } catch (e) {
          const code = TizenSockets.errno(e, nativeId);
          if (code === {{{ cDefs.EAGAIN }}} && !nonblock) {
            const r = TizenSockets.waitWritable(fd, nativeId);
            if (r > 0) continue;
            return sent || (r < 0 ? r : -{{{ cDefs.EAGAIN }}});
          }
          return sent || -code;
        }
        sent += n;
        if (nonblock || !stream || sent >= data.length || n === 0) return sent;
      }
    },

    gather(iov, iovcnt) {
      let total = 0;
      for (let i = 0; i < iovcnt; i++) {
        total += {{{ makeGetValue('iov', `${C_STRUCTS.iovec.__size__} * i + ${C_STRUCTS.iovec.iov_len}`, 'i32') }}};
      }
      const out = new Uint8Array(total);
      let off = 0;
      for (let i = 0; i < iovcnt; i++) {
        const base = {{{ makeGetValue('iov', `${C_STRUCTS.iovec.__size__} * i + ${C_STRUCTS.iovec.iov_base}`, '*') }}};
        const len = {{{ makeGetValue('iov', `${C_STRUCTS.iovec.__size__} * i + ${C_STRUCTS.iovec.iov_len}`, 'i32') }}};
        out.set(HEAPU8.subarray(base, base + len), off);
        off += len;
      }
      return out;
    },

    scatter(iov, iovcnt, src) {
      let off = 0;
      for (let i = 0; i < iovcnt && off < src.length; i++) {
        const base = {{{ makeGetValue('iov', `${C_STRUCTS.iovec.__size__} * i + ${C_STRUCTS.iovec.iov_base}`, '*') }}};
        const len = {{{ makeGetValue('iov', `${C_STRUCTS.iovec.__size__} * i + ${C_STRUCTS.iovec.iov_len}`, 'i32') }}};
        const n = Math.min(len, src.length - off);
        HEAPU8.set(src.subarray(off, off + n), base);
        off += n;
      }
      return off;
    },

    iovTotal(iov, iovcnt) {
      let total = 0;
      for (let i = 0; i < iovcnt; i++) {
        total += {{{ makeGetValue('iov', `${C_STRUCTS.iovec.__size__} * i + ${C_STRUCTS.iovec.iov_len}`, 'i32') }}};
      }
      return total;
    },

    // Socket option names understood by SocketsManager, keyed by C level and
    // option number. kind: 'int', 'u8', 'timeval', 'linger', 'mreq', 'mreq6'.
    OPTIONS: {
      [{{{ cDefs.SOL_SOCKET }}}]: {
        level: 'sol_socket',
        2: ['so_reuseaddr', 'int'],
        3: ['so_type', 'int'],
        4: ['so_error', 'int'],
        6: ['so_broadcast', 'int'],
        7: ['so_sndbuf', 'int'],
        8: ['so_rcvbuf', 'int'],
        9: ['so_keepalive', 'int'],
        13: ['so_linger', 'linger'],
        {{{ cDefs.SO_REUSEPORT }}}: ['so_reuseport', 'int'],
        66: ['so_rcvtimeo', 'timeval'],
        67: ['so_sndtimeo', 'timeval'],
      },
      [{{{ cDefs.IPPROTO_TCP }}}]: {
        level: 'ipproto_tcp',
        1: ['tcp_nodelay', 'int'],
        4: ['tcp_keepidle', 'int'],
        5: ['tcp_keepintvl', 'int'],
        6: ['tcp_keepcnt', 'int'],
      },
      [{{{ cDefs.IPPROTO_IP }}}]: {
        level: 'ipproto_ip',
        2: ['ip_ttl', 'int'],
        33: ['ip_multicast_ttl', 'u8'],
        34: ['ip_multicast_loop', 'u8'],
        35: ['ip_add_membership', 'mreq'],
        36: ['ip_drop_membership', 'mreq'],
      },
      [{{{ cDefs.IPPROTO_IPV6 }}}]: {
        level: 'ipproto_ipv6',
        18: ['ipv6_multicast_hops', 'int'],
        19: ['ipv6_multicast_loop', 'int'],
        20: ['ipv6_join_group', 'mreq6'],
        21: ['ipv6_leave_group', 'mreq6'],
        {{{ cDefs.IPV6_V6ONLY }}}: ['ipv6_v6only', 'int'],
      },
    },

    option(level, optname) {
      const lvl = TizenSockets.OPTIONS[level];
      const opt = lvl && lvl[optname];
      return opt ? { level: lvl.level, name: opt[0], kind: opt[1] } : null;
    },

    setsockopt(fd, nativeId, level, optname, optval, optlen) {
      const opt = TizenSockets.option(level, optname);
      if (!opt) return -{{{ cDefs.ENOPROTOOPT }}};
      if (!optval) return -{{{ cDefs.EFAULT }}};
      let value;
      switch (opt.kind) {
        case 'int':
          if (optlen < 1) return -{{{ cDefs.EINVAL }}};
          value = optlen >= 4 ? {{{ makeGetValue('optval', 0, 'i32') }}} : HEAPU8[optval];
          break;
        case 'u8':
          if (optlen < 1) return -{{{ cDefs.EINVAL }}};
          value = optlen >= 4 ? {{{ makeGetValue('optval', 0, 'i32') }}} & 0xff : HEAPU8[optval];
          break;
        case 'timeval': {
          if (optlen < 16) return -{{{ cDefs.EINVAL }}};
          const sec = {{{ makeGetValue('optval', 0, 'i32') }}};
          const usec = {{{ makeGetValue('optval', 8, 'i32') }}};
          TizenSockets.setTimeout(fd, optname === 66 ? TizenSockets.RCVTIMEO : TizenSockets.SNDTIMEO,
                                  sec * 1000 + Math.floor(usec / 1000));
          value = new tizentvwasm.SockOptTimeVal(sec, usec);
          break;
        }
        case 'linger':
          if (optlen < 8) return -{{{ cDefs.EINVAL }}};
          value = new tizentvwasm.SockOptSoLinger({{{ makeGetValue('optval', 0, 'i32') }}},
                                                  {{{ makeGetValue('optval', 4, 'i32') }}});
          break;
        case 'mreq':
          if (optlen < 8) return -{{{ cDefs.EINVAL }}};
          value = new tizentvwasm.IpMreq(new tizentvwasm.InetAddress(HEAPU8.slice(optval, optval + 4)),
                                         new tizentvwasm.InetAddress(HEAPU8.slice(optval + 4, optval + 8)));
          break;
        case 'mreq6':
          if (optlen < 16) return -{{{ cDefs.EINVAL }}};
          value = new tizentvwasm.Ipv6Mreq(new tizentvwasm.InetAddress(HEAPU8.slice(optval, optval + 16)));
          break;
      }
      try {
        tizentvwasm.SocketsManager.setSockOpt(nativeId, opt.level, opt.name, value);
        return 0;
      } catch (e) {
        // Timeouts are honoured by the shim even when the runtime rejects them.
        return opt.kind === 'timeval' ? 0 : -TizenSockets.errno(e, nativeId);
      }
    },

    getsockopt(fd, nativeId, level, optname, optval, optlenPtr) {
      const opt = TizenSockets.option(level, optname);
      if (!opt) return -{{{ cDefs.ENOPROTOOPT }}};
      if (!optval || !optlenPtr) return -{{{ cDefs.EFAULT }}};
      const capacity = {{{ makeGetValue('optlenPtr', 0, 'i32') }}};
      const writeInt = (v) => {
        if (capacity < 4) return -{{{ cDefs.EINVAL }}};
        {{{ makeSetValue('optval', 0, 'v', 'i32') }}};
        {{{ makeSetValue('optlenPtr', 0, 4, 'i32') }}};
        return 0;
      };
      if (opt.kind === 'timeval') {
        if (capacity < 16) return -{{{ cDefs.EINVAL }}};
        const field = optname === 66 ? TizenSockets.RCVTIMEO : TizenSockets.SNDTIMEO;
        const ms = Math.max(0, Atomics.load(HEAP32, TizenSockets.slot(fd, field)));
        {{{ makeSetValue('optval', 0, 'Math.floor(ms / 1000)', 'i64') }}};
        {{{ makeSetValue('optval', 8, '(ms % 1000) * 1000', 'i32') }}};
        {{{ makeSetValue('optlenPtr', 0, 16, 'i32') }}};
        return 0;
      }
      if (opt.name === 'so_type') {
        return writeInt(TizenSockets.flags(fd) & TizenSockets.FLAG_DGRAM ? {{{ cDefs.SOCK_DGRAM }}}
                                                                          : {{{ cDefs.SOCK_STREAM }}});
      }
      let value;
      try {
        value = tizentvwasm.SocketsManager.getSockOpt(nativeId, opt.level, opt.name);
      } catch (e) {
        return -TizenSockets.errno(e, nativeId);
      }
      if (opt.name === 'so_error') return writeInt(value ? _kodi_sock_errno_from_linux(Number(value)) : 0);
      if (opt.kind === 'linger') {
        if (capacity < 8) return -{{{ cDefs.EINVAL }}};
        {{{ makeSetValue('optval', 0, 'value.onoff | 0', 'i32') }}};
        {{{ makeSetValue('optval', 4, 'value.linger | 0', 'i32') }}};
        {{{ makeSetValue('optlenPtr', 0, 8, 'i32') }}};
        return 0;
      }
      return writeInt(Number(value) | 0);
    },

    fcntl(fd, cmd, varargs) {
      switch (cmd) {
        case {{{ cDefs.F_GETFL }}}:
          return 2 /* O_RDWR */ |
                 (TizenSockets.flags(fd) & TizenSockets.FLAG_NONBLOCK ? {{{ cDefs.O_NONBLOCK }}} : 0);
        case {{{ cDefs.F_SETFL }}}: {
          const arg = {{{ makeGetValue('varargs', 0, 'i32') }}};
          let flags = TizenSockets.flags(fd) & ~TizenSockets.FLAG_NONBLOCK;
          if (arg & {{{ cDefs.O_NONBLOCK }}}) flags |= TizenSockets.FLAG_NONBLOCK;
          TizenSockets.setFlags(fd, flags);
          return 0;
        }
        case {{{ cDefs.F_GETFD }}}:
        case {{{ cDefs.F_SETFD }}}:
          return 0;
        default:
          return -{{{ cDefs.EINVAL }}};
      }
    },

    ioctl(fd, op, varargs) {
      if (op === {{{ cDefs.FIONBIO }}}) {
        const argp = {{{ makeGetValue('varargs', 0, '*') }}};
        const on = argp ? {{{ makeGetValue('argp', 0, 'i32') }}} : 0;
        let flags = TizenSockets.flags(fd) & ~TizenSockets.FLAG_NONBLOCK;
        if (on) flags |= TizenSockets.FLAG_NONBLOCK;
        TizenSockets.setFlags(fd, flags);
        return 0;
      }
      return -{{{ cDefs.ENOTTY }}};
    },

    // poll() over the socket entries of a pollfd array. Writes revents in
    // place and returns the number of ready descriptors or a negative errno.
    pollSockets(fds, indices, timeoutMs) {
      const pollFds = [];
      for (const i of indices) {
        const p = fds + i * {{{ C_STRUCTS.pollfd.__size__ }}};
        const fd = {{{ makeGetValue('p', C_STRUCTS.pollfd.fd, 'i32') }}};
        const events = {{{ makeGetValue('p', C_STRUCTS.pollfd.events, 'i16') }}};
        pollFds.push(new tizentvwasm.PollFd(TizenSockets.native(fd), TizenSockets.pollToJs(events)));
      }
      try {
        tizentvwasm.SocketsManager.poll(pollFds, timeoutMs);
      } catch (e) {
        return -TizenSockets.errno(e, -1);
      }
      let ready = 0;
      for (let k = 0; k < indices.length; k++) {
        const p = fds + indices[k] * {{{ C_STRUCTS.pollfd.__size__ }}};
        const revents = TizenSockets.pollFromJs(pollFds[k].revents);
        {{{ makeSetValue('p', C_STRUCTS.pollfd.revents, 'revents', 'i16') }}};
        if (revents) ready++;
      }
      return ready;
    },

    // Zero-timeout poll of the non-socket entries through the upstream
    // implementation on the main thread, using a temporary pollfd array.
    pollFilesNow(fds, indices) {
      const tmp = _malloc(indices.length * {{{ C_STRUCTS.pollfd.__size__ }}});
      if (!tmp) return -{{{ cDefs.ENOMEM }}};
      for (let k = 0; k < indices.length; k++) {
        const src = fds + indices[k] * {{{ C_STRUCTS.pollfd.__size__ }}};
        const dst = tmp + k * {{{ C_STRUCTS.pollfd.__size__ }}};
        HEAPU8.copyWithin(dst, src, src + {{{ C_STRUCTS.pollfd.__size__ }}});
      }
      const ready = _kodi_upstream_poll_nonblocking(tmp, indices.length);
      for (let k = 0; k < indices.length; k++) {
        const src = tmp + k * {{{ C_STRUCTS.pollfd.__size__ }}};
        const dst = fds + indices[k] * {{{ C_STRUCTS.pollfd.__size__ }}};
        const revents = {{{ makeGetValue('src', C_STRUCTS.pollfd.revents, 'i16') }}};
        {{{ makeSetValue('dst', C_STRUCTS.pollfd.revents, 'revents', 'i16') }}};
      }
      _free(tmp);
      return ready;
    },

    classify(fds, nfds) {
      const sockets = [], files = [];
      for (let i = 0; i < nfds; i++) {
        const p = fds + i * {{{ C_STRUCTS.pollfd.__size__ }}};
        const fd = {{{ makeGetValue('p', C_STRUCTS.pollfd.fd, 'i32') }}};
        (TizenSockets.native(fd) >= 0 ? sockets : files).push(i);
      }
      return { sockets, files };
    },

    pollMixed(fds, sockets, files, timeoutMs) {
      const deadline = timeoutMs < 0 ? Infinity : performance.now() + timeoutMs;
      for (;;) {
        const remaining = deadline - performance.now();
        const slice = remaining === Infinity ? TizenSockets.MIXED_POLL_SLICE_MS
                                             : Math.max(0, Math.min(TizenSockets.MIXED_POLL_SLICE_MS, Math.ceil(remaining)));
        const s = TizenSockets.pollSockets(fds, sockets, slice);
        if (s < 0) return s;
        const f = TizenSockets.pollFilesNow(fds, files);
        if (f < 0) return f;
        if (s + f > 0 || performance.now() >= deadline) return s + f;
      }
    },
  },

  // Reserve and release Emscripten fds for sockets. The FS lives on the main
  // thread, so these run there; the placeholder stream keeps the fd out of the
  // file descriptor space and has no I/O operations of its own.
  _kodi_sock_fd_reserve__proxy: 'sync',
  _kodi_sock_fd_reserve__sig: 'i',
  _kodi_sock_fd_reserve__deps: ['$FS'],
  _kodi_sock_fd_reserve: () => {
    try {
      const node = FS.createNode(null, 'tizen-socket', {{{ cDefs.S_IFSOCK }}} | 0o666, 0);
      const stream = FS.createStream({ path: 'tizen-socket', node, flags: 2, seekable: false, stream_ops: {} });
      return stream.fd;
    } catch (e) {
      return -(e.errno || {{{ cDefs.EMFILE }}});
    }
  },

  _kodi_sock_fd_release__proxy: 'sync',
  _kodi_sock_fd_release__sig: 'vi',
  _kodi_sock_fd_release__deps: ['$FS'],
  _kodi_sock_fd_release: (fd) => {
    const stream = FS.getStream(fd);
    if (stream) {
      FS.destroyNode(stream.node);
      FS.closeStream(fd);
    }
  },

  // Called once by kodi_wasm_has_sockets(); the result is cached in C.
  kodi_tizen_sockets_probe__proxy: 'none',
  kodi_tizen_sockets_probe__sig: 'i',
  kodi_tizen_sockets_probe__deps: ['$TizenSockets'],
  kodi_tizen_sockets_probe: () => TizenSockets.available() ? 1 : 0,

  __syscall_socket__deps: ['$TizenSockets', 'kodi_upstream_socket'],
  __syscall_socket__proxy: 'none',
  __syscall_socket: (domain, type, protocol, u1, u2, u3) => {
    if (!TizenSockets.available()) return _kodi_upstream_socket(domain, type, protocol);
    return TizenSockets.socket(domain, type, protocol);
  },

  __syscall_bind__deps: ['$TizenSockets', 'kodi_upstream_bind'],
  __syscall_bind__proxy: 'none',
  __syscall_bind: (fd, addr, len, u1, u2, u3) => {
    const n = TizenSockets.native(fd);
    if (n < 0) return _kodi_upstream_bind(fd, addr, len);
    return TizenSockets.bind(n, addr, len);
  },

  __syscall_connect__deps: ['$TizenSockets', 'kodi_upstream_connect'],
  __syscall_connect__proxy: 'none',
  __syscall_connect: (fd, addr, len, u1, u2, u3) => {
    const n = TizenSockets.native(fd);
    if (n < 0) return _kodi_upstream_connect(fd, addr, len);
    return TizenSockets.connect(fd, n, addr, len);
  },

  __syscall_listen__deps: ['$TizenSockets', 'kodi_upstream_listen'],
  __syscall_listen__proxy: 'none',
  __syscall_listen: (fd, backlog, u1, u2, u3, u4) => {
    const n = TizenSockets.native(fd);
    if (n < 0) return _kodi_upstream_listen(fd, backlog);
    return TizenSockets.listen(n, backlog);
  },

  __syscall_accept4__deps: ['$TizenSockets', 'kodi_upstream_accept4'],
  __syscall_accept4__proxy: 'none',
  __syscall_accept4: (fd, addr, len, flags, u1, u2) => {
    const n = TizenSockets.native(fd);
    if (n < 0) return _kodi_upstream_accept4(fd, addr, len, flags);
    return TizenSockets.accept4(fd, n, addr, len, flags);
  },

  __syscall_getsockname__deps: ['$TizenSockets', 'kodi_upstream_getsockname'],
  __syscall_getsockname__proxy: 'none',
  __syscall_getsockname: (fd, addr, len, u1, u2, u3) => {
    const n = TizenSockets.native(fd);
    if (n < 0) return _kodi_upstream_getsockname(fd, addr, len);
    return TizenSockets.name(n, addr, len, false);
  },

  __syscall_getpeername__deps: ['$TizenSockets', 'kodi_upstream_getpeername'],
  __syscall_getpeername__proxy: 'none',
  __syscall_getpeername: (fd, addr, len, u1, u2, u3) => {
    const n = TizenSockets.native(fd);
    if (n < 0) return _kodi_upstream_getpeername(fd, addr, len);
    return TizenSockets.name(n, addr, len, true);
  },

  __syscall_sendto__deps: ['$TizenSockets', 'kodi_upstream_sendto'],
  __syscall_sendto__proxy: 'none',
  __syscall_sendto: (fd, buf, len, flags, addr, alen) => {
    const n = TizenSockets.native(fd);
    if (n < 0) return _kodi_upstream_sendto(fd, buf, len, flags, addr, alen);
    return TizenSockets.send(fd, n, HEAPU8.subarray(buf, buf + len), flags, addr, alen);
  },

  __syscall_recvfrom__deps: ['$TizenSockets', 'kodi_upstream_recvfrom'],
  __syscall_recvfrom__proxy: 'none',
  __syscall_recvfrom: (fd, buf, len, flags, addr, alen) => {
    const n = TizenSockets.native(fd);
    if (n < 0) return _kodi_upstream_recvfrom(fd, buf, len, flags, addr, alen);
    return TizenSockets.recv(fd, n, HEAPU8.subarray(buf, buf + len), flags, addr, alen);
  },

  __syscall_sendmsg__deps: ['$TizenSockets', 'kodi_upstream_sendmsg'],
  __syscall_sendmsg__proxy: 'none',
  __syscall_sendmsg: (fd, message, flags, u1, u2, u3) => {
    const n = TizenSockets.native(fd);
    if (n < 0) return _kodi_upstream_sendmsg(fd, message, flags);
    const iov = {{{ makeGetValue('message', C_STRUCTS.msghdr.msg_iov, '*') }}};
    const iovcnt = {{{ makeGetValue('message', C_STRUCTS.msghdr.msg_iovlen, 'i32') }}};
    const name = {{{ makeGetValue('message', C_STRUCTS.msghdr.msg_name, '*') }}};
    const namelen = {{{ makeGetValue('message', C_STRUCTS.msghdr.msg_namelen, 'i32') }}};
    return TizenSockets.send(fd, n, TizenSockets.gather(iov, iovcnt), flags, name, namelen);
  },

  __syscall_recvmsg__deps: ['$TizenSockets', 'kodi_upstream_recvmsg'],
  __syscall_recvmsg__proxy: 'none',
  __syscall_recvmsg: (fd, message, flags, u1, u2, u3) => {
    const n = TizenSockets.native(fd);
    if (n < 0) return _kodi_upstream_recvmsg(fd, message, flags);
    const iov = {{{ makeGetValue('message', C_STRUCTS.msghdr.msg_iov, '*') }}};
    const iovcnt = {{{ makeGetValue('message', C_STRUCTS.msghdr.msg_iovlen, 'i32') }}};
    const name = {{{ makeGetValue('message', C_STRUCTS.msghdr.msg_name, '*') }}};
    const buffer = new Uint8Array(TizenSockets.iovTotal(iov, iovcnt));
    const got = TizenSockets.recv(fd, n, buffer, flags, name, name ? message + {{{ C_STRUCTS.msghdr.msg_namelen }}} : 0);
    if (got < 0) return got;
    TizenSockets.scatter(iov, iovcnt, buffer.subarray(0, got));
    {{{ makeSetValue('message', C_STRUCTS.msghdr.msg_controllen, '0', 'i32') }}};
    {{{ makeSetValue('message', C_STRUCTS.msghdr.msg_flags, '0', 'i32') }}};
    return got;
  },

  __syscall_getsockopt__deps: ['$TizenSockets', 'kodi_upstream_getsockopt'],
  __syscall_getsockopt__proxy: 'none',
  __syscall_getsockopt: (fd, level, optname, optval, optlen, unused) => {
    const n = TizenSockets.native(fd);
    if (n < 0) return _kodi_upstream_getsockopt(fd, level, optname, optval, optlen);
    return TizenSockets.getsockopt(fd, n, level, optname, optval, optlen);
  },

  __syscall_setsockopt__deps: ['$TizenSockets', 'kodi_upstream_setsockopt'],
  __syscall_setsockopt__proxy: 'none',
  __syscall_setsockopt: (fd, level, optname, optval, optlen, unused) => {
    const n = TizenSockets.native(fd);
    if (n < 0) return _kodi_upstream_setsockopt(fd, level, optname, optval, optlen);
    return TizenSockets.setsockopt(fd, n, level, optname, optval, optlen);
  },

  __syscall_shutdown__deps: ['$TizenSockets', 'kodi_upstream_shutdown'],
  __syscall_shutdown__proxy: 'none',
  __syscall_shutdown: (fd, how, u1, u2, u3, u4) => {
    const n = TizenSockets.native(fd);
    if (n < 0) return _kodi_upstream_shutdown(fd, how);
    return TizenSockets.shutdown(n, how);
  },

  __syscall_poll__deps: ['$TizenSockets', 'kodi_upstream_poll'],
  __syscall_poll__proxy: 'none',
  __syscall_poll__async: false,
  __syscall_poll: (fds, nfds, timeout) => {
    const { sockets, files } = TizenSockets.classify(fds, nfds);
    if (!sockets.length) return _kodi_upstream_poll(fds, nfds, timeout);
    if (!files.length) return TizenSockets.pollSockets(fds, sockets, timeout);
    return TizenSockets.pollMixed(fds, sockets, files, timeout);
  },

  __syscall_poll_nonblocking__deps: ['$TizenSockets', 'kodi_upstream_poll_nonblocking'],
  __syscall_poll_nonblocking__proxy: 'none',
  __syscall_poll_nonblocking: (fds, nfds) => {
    const { sockets, files } = TizenSockets.classify(fds, nfds);
    if (!sockets.length) return _kodi_upstream_poll_nonblocking(fds, nfds);
    const s = TizenSockets.pollSockets(fds, sockets, 0);
    if (s < 0 || !files.length) return s;
    const f = TizenSockets.pollFilesNow(fds, files);
    return f < 0 ? f : s + f;
  },

  __syscall_fcntl64__deps: ['$TizenSockets', 'kodi_upstream_fcntl64'],
  __syscall_fcntl64__proxy: 'none',
  __syscall_fcntl64: (fd, cmd, varargs) => {
    if (TizenSockets.native(fd) < 0) return _kodi_upstream_fcntl64(fd, cmd, varargs);
    return TizenSockets.fcntl(fd, cmd, varargs);
  },

  __syscall_ioctl__deps: ['$TizenSockets', 'kodi_upstream_ioctl'],
  __syscall_ioctl__proxy: 'none',
  __syscall_ioctl: (fd, op, varargs) => {
    if (TizenSockets.native(fd) < 0) return _kodi_upstream_ioctl(fd, op, varargs);
    return TizenSockets.ioctl(fd, op, varargs);
  },

  // WASI entry points return a positive errno.
  fd_close__deps: ['$TizenSockets', 'kodi_upstream_fd_close'],
  fd_close__proxy: 'none',
  fd_close: (fd) => {
    const n = TizenSockets.native(fd);
    if (n < 0) return _kodi_upstream_fd_close(fd);
    return -TizenSockets.close(fd, n);
  },

  fd_read__deps: ['$TizenSockets', 'kodi_upstream_fd_read'],
  fd_read__proxy: 'none',
  fd_read: (fd, iov, iovcnt, pnum) => {
    const n = TizenSockets.native(fd);
    if (n < 0) return _kodi_upstream_fd_read(fd, iov, iovcnt, pnum);
    const buffer = new Uint8Array(TizenSockets.iovTotal(iov, iovcnt));
    const got = TizenSockets.recv(fd, n, buffer, 0, 0, 0);
    if (got < 0) return -got;
    TizenSockets.scatter(iov, iovcnt, buffer.subarray(0, got));
    {{{ makeSetValue('pnum', 0, 'got', '*') }}};
    return 0;
  },

  fd_write__deps: ['$TizenSockets', 'kodi_upstream_fd_write'],
  fd_write__proxy: 'none',
  fd_write: (fd, iov, iovcnt, pnum) => {
    const n = TizenSockets.native(fd);
    if (n < 0) return _kodi_upstream_fd_write(fd, iov, iovcnt, pnum);
    const sent = TizenSockets.send(fd, n, TizenSockets.gather(iov, iovcnt), 0, 0, 0);
    if (sent < 0) return -sent;
    {{{ makeSetValue('pnum', 0, 'sent', '*') }}};
    return 0;
  },

  getaddrinfo__deps: ['$TizenSockets', 'kodi_upstream_getaddrinfo'],
  getaddrinfo__proxy: 'none',
  getaddrinfo: (node, service, hint, out) => {
    if (!TizenSockets.available()) return _kodi_upstream_getaddrinfo(node, service, hint, out);

    let flags = 0, family = {{{ cDefs.AF_UNSPEC }}}, type = 0, proto = 0;
    if (hint) {
      flags = {{{ makeGetValue('hint', C_STRUCTS.addrinfo.ai_flags, 'i32') }}};
      family = {{{ makeGetValue('hint', C_STRUCTS.addrinfo.ai_family, 'i32') }}};
      type = {{{ makeGetValue('hint', C_STRUCTS.addrinfo.ai_socktype, 'i32') }}};
      proto = {{{ makeGetValue('hint', C_STRUCTS.addrinfo.ai_protocol, 'i32') }}};
    }
    if (type && !proto) proto = type === {{{ cDefs.SOCK_DGRAM }}} ? {{{ cDefs.IPPROTO_UDP }}} : {{{ cDefs.IPPROTO_TCP }}};
    if (!type && proto) type = proto === {{{ cDefs.IPPROTO_UDP }}} ? {{{ cDefs.SOCK_DGRAM }}} : {{{ cDefs.SOCK_STREAM }}};
    if (!proto) proto = {{{ cDefs.IPPROTO_TCP }}};
    if (!type) type = {{{ cDefs.SOCK_STREAM }}};

    if (!node && !service) return {{{ cDefs.EAI_NONAME }}};
    if (type !== {{{ cDefs.SOCK_STREAM }}} && type !== {{{ cDefs.SOCK_DGRAM }}}) return {{{ cDefs.EAI_SOCKTYPE }}};
    if (family !== {{{ cDefs.AF_UNSPEC }}} && family !== {{{ cDefs.AF_INET }}} && family !== {{{ cDefs.AF_INET6 }}}) {
      return {{{ cDefs.EAI_FAMILY }}};
    }
    if ((flags & {{{ cDefs.AI_CANONNAME }}}) && !node) return {{{ cDefs.EAI_BADFLAGS }}};

    let port = 0;
    if (service) {
      const text = UTF8ToString(service);
      if (!/^\d+$/.test(text)) return (flags & {{{ cDefs.AI_NUMERICSERV }}}) ? {{{ cDefs.EAI_NONAME }}} : {{{ cDefs.EAI_SERVICE }}};
      port = parseInt(text, 10);
      if (port > 65535) return {{{ cDefs.EAI_SERVICE }}};
    }

    const addrs = [];
    if (!node) {
      const af = family === {{{ cDefs.AF_UNSPEC }}} ? {{{ cDefs.AF_INET }}} : family;
      const bytes = new Uint8Array(af === {{{ cDefs.AF_INET6 }}} ? 16 : 4);
      if (!(flags & {{{ cDefs.AI_PASSIVE }}})) {
        if (af === {{{ cDefs.AF_INET6 }}}) bytes[15] = 1; else bytes.set([127, 0, 0, 1]);
      }
      addrs.push({ family: af, bytes });
    } else {
      const name = UTF8ToString(node);
      const v4 = inetPton4(name);
      const v6 = v4 === null ? inetPton6(name) : null;
      if (v4 !== null) {
        if (family === {{{ cDefs.AF_INET6 }}}) return {{{ cDefs.EAI_NONAME }}};
        addrs.push({ family: {{{ cDefs.AF_INET }}}, bytes: new Uint8Array(new Uint32Array([v4]).buffer) });
      } else if (v6 !== null) {
        if (family === {{{ cDefs.AF_INET }}}) return {{{ cDefs.EAI_NONAME }}};
        addrs.push({ family: {{{ cDefs.AF_INET6 }}}, bytes: new Uint8Array(new Uint32Array(v6).buffer) });
      } else {
        if (flags & {{{ cDefs.AI_NUMERICHOST }}}) return {{{ cDefs.EAI_NONAME }}};
        let host;
        try {
          host = new tizentvwasm.HostResolverSync().getHostByName(name);
        } catch (e) {
          return -4; /* EAI_FAIL */
        }
        for (const a of Array.from(host.getAddrList())) {
          const af = a.family === 'af_inet6' ? {{{ cDefs.AF_INET6 }}} : {{{ cDefs.AF_INET }}};
          if (family === {{{ cDefs.AF_UNSPEC }}} || family === af) {
            addrs.push({ family: af, bytes: Uint8Array.from(a.bytes) });
          }
        }
        if (!addrs.length) return {{{ cDefs.EAI_NONAME }}};
      }
    }

    let head = 0, prev = 0;
    for (const a of addrs) {
      const v6 = a.family === {{{ cDefs.AF_INET6 }}};
      const salen = v6 ? {{{ C_STRUCTS.sockaddr_in6.__size__ }}} : {{{ C_STRUCTS.sockaddr_in.__size__ }}};
      const sa = _malloc(salen);
      const ai = _malloc({{{ C_STRUCTS.addrinfo.__size__ }}});
      if (!sa || !ai) {
        _free(sa); _free(ai);
        _freeaddrinfo(head);
        return -10; /* EAI_MEMORY */
      }
      HEAPU8.fill(0, sa, sa + salen);
      {{{ makeSetValue('sa', C_STRUCTS.sockaddr_in.sin_family, 'a.family', 'i16') }}};
      HEAPU8[sa + {{{ C_STRUCTS.sockaddr_in.sin_port }}}] = port >> 8;
      HEAPU8[sa + {{{ C_STRUCTS.sockaddr_in.sin_port }}} + 1] = port & 0xff;
      HEAPU8.set(a.bytes, sa + (v6 ? {{{ C_STRUCTS.sockaddr_in6.sin6_addr.__in6_union.__s6_addr }}}
                                   : {{{ C_STRUCTS.sockaddr_in.sin_addr.s_addr }}}));
      HEAPU8.fill(0, ai, ai + {{{ C_STRUCTS.addrinfo.__size__ }}});
      {{{ makeSetValue('ai', C_STRUCTS.addrinfo.ai_flags, 'flags', 'i32') }}};
      {{{ makeSetValue('ai', C_STRUCTS.addrinfo.ai_family, 'a.family', 'i32') }}};
      {{{ makeSetValue('ai', C_STRUCTS.addrinfo.ai_socktype, 'type', 'i32') }}};
      {{{ makeSetValue('ai', C_STRUCTS.addrinfo.ai_protocol, 'proto', 'i32') }}};
      {{{ makeSetValue('ai', C_STRUCTS.addrinfo.ai_addrlen, 'salen', 'i32') }}};
      {{{ makeSetValue('ai', C_STRUCTS.addrinfo.ai_addr, 'sa', '*') }}};
      // ai_canonname stays NULL, as with Emscripten's own getaddrinfo():
      // its freeaddrinfo() would not free it.
      if (prev) {
        {{{ makeSetValue('prev', C_STRUCTS.addrinfo.ai_next, 'ai', '*') }}};
      } else {
        head = ai;
      }
      prev = ai;
    }
    {{{ makeSetValue('out', 0, 'head', '*') }}};
    return 0;
  },
});
