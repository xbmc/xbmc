/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "OSScreenSaverWasm.h"

#include <emscripten.h>
#include <emscripten/em_asm.h>

using namespace KODI::WINDOWING::WASM;

namespace
{
// The browser releases a wake lock whenever the page is hidden, so the lock is
// re-requested on the next visibility change for as long as it is wanted.
void SetWakeLockWanted(bool wanted)
{
  // clang-format off
  MAIN_THREAD_EM_ASM(({
    if (!('wakeLock' in navigator))
      return;
    const kodi = (Module.kodi = Module.kodi || {});
    if (!kodi.wakeLock) {
      const wl = (kodi.wakeLock = { wanted: false, pending: false, sentinel: null, warned: false });
      wl.acquire = () => {
        if (!wl.wanted || wl.pending || wl.sentinel || document.visibilityState !== 'visible')
          return;
        wl.pending = true;
        navigator.wakeLock.request('screen').then((sentinel) => {
          wl.pending = false;
          if (!wl.wanted) {
            sentinel.release();
            return;
          }
          wl.sentinel = sentinel;
          sentinel.addEventListener('release', () => {
            if (wl.sentinel === sentinel)
              wl.sentinel = null;
          });
        }).catch((e) => {
          wl.pending = false;
          if (!wl.warned) {
            wl.warned = true;
            console.warn('[kodi] screen wake lock:', e);
          }
        });
      };
      document.addEventListener('visibilitychange', wl.acquire);
    }
    const wl = kodi.wakeLock;
    wl.wanted = !!$0;
    if (wl.wanted) {
      wl.acquire();
    } else if (wl.sentinel) {
      wl.sentinel.release();
      wl.sentinel = null;
    }
  }), wanted ? 1 : 0);
  // clang-format on
}
} // namespace

void COSScreenSaverWasm::Inhibit()
{
  SetWakeLockWanted(true);
}

void COSScreenSaverWasm::Uninhibit()
{
  SetWakeLockWanted(false);
}
