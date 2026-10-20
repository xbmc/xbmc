/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

/* Largest fd the socket table tracks; matches Emscripten's FS.MAX_OPEN_FDS. */
#define KODI_SOCK_MAX_FD 4096

  /*! \brief Whether BSD sockets are backed by the Tizen Sockets Extension.
   *
   * Samsung TVs expose TCP/UDP sockets and synchronous DNS to WebAssembly through
   * `tizentvwasm`; elsewhere sockets keep Emscripten's default behaviour. The
   * result is cached after the first call and is the same on every thread.
   */
  int kodi_wasm_has_sockets(void);

#ifdef __cplusplus
}
#endif
