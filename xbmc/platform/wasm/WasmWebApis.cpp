/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WasmWebApis.h"

#include "TitanWebApis.h"
#include "TizenWebApis.h"

#include <emscripten/em_asm.h>

namespace KODI::PLATFORM::WASM
{
WasmDeviceInfo CWasmWebApis::GetDeviceInfo()
{
  // The launcher selects the provider and finishes SDK initialization before Kodi starts.
  static const WasmDeviceInfo info = []
  {
    // clang-format off
    const int provider = MAIN_THREAD_EM_ASM_INT(({
      const provider = Module.kodi && Module.kodi.webApiProvider;
      if (provider === 'tizen') return 1;
      if (provider === 'titan') return 2;
      return 0;
    }));
    // clang-format on
    switch (provider)
    {
      case 1:
        return CTizenWebApis::GetDeviceInfo();
      case 2:
        return CTitanWebApis::GetDeviceInfo();
      default:
        return WasmDeviceInfo{};
    }
  }();
  return info;
}
} // namespace KODI::PLATFORM::WASM
