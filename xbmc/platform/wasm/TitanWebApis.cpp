/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "TitanWebApis.h"

#include "utils/JSONVariantParser.h"
#include "utils/Variant.h"

#include <cstdlib>

#include <emscripten/em_asm.h>

namespace KODI::PLATFORM::WASM
{
WasmDeviceInfo CTitanWebApis::GetDeviceInfo()
{
  WasmDeviceInfo info;

  // clang-format off
  void* json = MAIN_THREAD_EM_ASM_PTR(({
    const info = Module.kodi && Module.kodi.titanDeviceInfo;
    return info ? stringToNewUTF8(JSON.stringify(info)) : 0;
  }));
  // clang-format on
  if (!json)
    return info;

  CVariant data;
  const bool parsed = CJSONVariantParser::Parse(static_cast<const char*>(json), data);
  std::free(json);
  if (!parsed || !data.isObject())
    return info;

  info.available = true;
  info.osName = "TitanOS";
  info.manufacturer = data["manufacturer"].asString();
  info.model = data["model"].asString();
  info.firmware = data["firmware"].asString();
  info.platformVersion = data["platformVersion"].asString();
  info.macAddress = data["macAddress"].asString();
  return info;
}
} // namespace KODI::PLATFORM::WASM
