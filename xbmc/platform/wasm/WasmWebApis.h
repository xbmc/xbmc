/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

namespace KODI::PLATFORM::WASM
{
struct WasmDeviceInfo
{
  bool available{false};
  std::string osName;
  std::string manufacturer;
  std::string model;
  std::string firmware;
  std::string platformVersion;
  std::string macAddress;
};

class CWasmWebApis
{
public:
  static WasmDeviceInfo GetDeviceInfo();
};
} // namespace KODI::PLATFORM::WASM
