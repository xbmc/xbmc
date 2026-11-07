/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "WasmWebApis.h"

namespace KODI::PLATFORM::WASM
{
class CTitanWebApis
{
public:
  static WasmDeviceInfo GetDeviceInfo();
};
} // namespace KODI::PLATFORM::WASM
