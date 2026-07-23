/*
 *  Copyright (C) 2018-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SocketOptions.h"

#include "PlatformDefs.h"

#if !defined(TARGET_WASM)
#if defined(WINSOCK_VERSION)
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#include <sys/socket.h>
#endif
#endif

namespace KODI::NETWORK
{
#if defined(TARGET_WASM)

// Emscripten's BSD socket layer does not implement these; skipping matches the
// previous bare #ifndef TARGET_WASM at call sites, without replicating ifdefs
// in each caller.

bool SetReusePort(SOCKET sock)
{
  (void)sock;
  return true;
}

bool SetIPv6Only(SOCKET sock, int v6only)
{
  (void)sock;
  (void)v6only;
  return true;
}

#else

bool SetReusePort(SOCKET sock)
{
#if defined(WINSOCK_VERSION)
  int yes = 1;
  return setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&yes),
                    sizeof(yes)) == 0;
#else
  int yes = 1;
  return setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == 0;
#endif
}

bool SetIPv6Only(SOCKET sock, int v6only)
{
#if defined(WINSOCK_VERSION)
  int opt = v6only;
  return setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast<const char*>(&opt),
                    sizeof(opt)) == 0;
#else
  int opt = v6only;
  return setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt)) == 0;
#endif
}

#endif
} // namespace KODI::NETWORK
