/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

class CVariant;

namespace KODI::PLATFORM::WASM
{
enum class WebApiProvider
{
  NONE,
  TIZEN,
  TITAN,
};

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
  static WebApiProvider GetProvider();
  static WasmDeviceInfo GetDeviceInfo();
};

// Takes ownership of a stringToNewUTF8() result (nullptr when the JS side failed).
bool ParseOwnedJson(void* ptr, CVariant& out);
} // namespace KODI::PLATFORM::WASM
