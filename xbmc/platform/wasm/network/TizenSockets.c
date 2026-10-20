/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "TizenSockets.h"

#include <errno.h>
#include <stdatomic.h>
#include <stdint.h>

#include <emscripten.h>

/* Per-fd state shared by every pthread. The JS side (tizen_sockets.js) reads
 * and writes it with Atomics through the address returned below. Entries are
 * indexed by the Emscripten fd; `native` holds the Tizen socket id plus one so
 * that zero means "not a socket". */
struct kodi_sock_entry
{
  _Atomic int32_t native;
  _Atomic int32_t flags;
  _Atomic int32_t rcvtimeo_ms;
  _Atomic int32_t sndtimeo_ms;
};

static struct kodi_sock_entry g_sockets[KODI_SOCK_MAX_FD];

/* 0 = unknown, 1 = available, 2 = unavailable. */
static _Atomic int32_t g_availability;

extern int kodi_tizen_sockets_probe(void);

EMSCRIPTEN_KEEPALIVE void* kodi_sock_table(void)
{
  return g_sockets;
}

EMSCRIPTEN_KEEPALIVE int kodi_sock_max_fd(void)
{
  return KODI_SOCK_MAX_FD;
}

/* The Tizen runtime reports Linux errno values; libc here uses its own. */
EMSCRIPTEN_KEEPALIVE int kodi_sock_errno_from_linux(int code)
{
  switch (code)
  {
    case 1:
      return EPERM;
    case 2:
      return ENOENT;
    case 4:
      return EINTR;
    case 5:
      return EIO;
    case 9:
      return EBADF;
    case 11:
      return EAGAIN;
    case 12:
      return ENOMEM;
    case 13:
      return EACCES;
    case 14:
      return EFAULT;
    case 16:
      return EBUSY;
    case 17:
      return EEXIST;
    case 22:
      return EINVAL;
    case 23:
      return ENFILE;
    case 24:
      return EMFILE;
    case 25:
      return ENOTTY;
    case 28:
      return ENOSPC;
    case 32:
      return EPIPE;
    case 38:
      return ENOSYS;
    case 88:
      return ENOTSOCK;
    case 89:
      return EDESTADDRREQ;
    case 90:
      return EMSGSIZE;
    case 91:
      return EPROTOTYPE;
    case 92:
      return ENOPROTOOPT;
    case 93:
      return EPROTONOSUPPORT;
    case 95:
      return EOPNOTSUPP;
    case 97:
      return EAFNOSUPPORT;
    case 98:
      return EADDRINUSE;
    case 99:
      return EADDRNOTAVAIL;
    case 100:
      return ENETDOWN;
    case 101:
      return ENETUNREACH;
    case 102:
      return ENETRESET;
    case 103:
      return ECONNABORTED;
    case 104:
      return ECONNRESET;
    case 105:
      return ENOBUFS;
    case 106:
      return EISCONN;
    case 107:
      return ENOTCONN;
    case 110:
      return ETIMEDOUT;
    case 111:
      return ECONNREFUSED;
    case 112:
      return EHOSTDOWN;
    case 113:
      return EHOSTUNREACH;
    case 114:
      return EALREADY;
    case 115:
      return EINPROGRESS;
    case 125:
      return ECANCELED;
    default:
      return EIO;
  }
}

int kodi_wasm_has_sockets(void)
{
  int32_t state = atomic_load(&g_availability);
  if (state == 0)
  {
    state = kodi_tizen_sockets_probe() ? 1 : 2;
    atomic_store(&g_availability, state);
  }
  return state == 1;
}
