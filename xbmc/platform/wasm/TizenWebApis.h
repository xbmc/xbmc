/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "WasmWebApis.h"

#include <string>

namespace KODI::PLATFORM::WASM
{

/*!
 * \brief Snapshot of the Samsung Tizen Network API (webapis.network).
 *
 * \c available is false when the page does not run in the Samsung web runtime
 * (e.g. a desktop browser); every other field is then left at its default.
 * String fields are empty when the TV reports no value, which is what the API
 * does while the TV is disconnected.
 */
struct TizenNetworkInfo
{
  enum class ConnectionType
  {
    DISCONNECTED,
    WIFI,
    CELLULAR,
    ETHERNET,
    UNKNOWN,
  };

  bool available{false};
  ConnectionType connectionType{ConnectionType::UNKNOWN};
  bool gatewayConnected{false};
  std::string ipMode;
  std::string ipAddress;
  std::string netmask;
  std::string gateway;
  std::string macAddress;
  std::string dns1;
  std::string dns2;
  std::string wifiSsid;
  int wifiSignalLevel{-1};
  int wifiFrequencyMHz{0};
  std::string tvName;
};

/*!
 * \brief Access to the Samsung Tizen web APIs from Kodi's pthread.
 *
 * The webapis object only exists on the browser main thread, so every query is
 * proxied there synchronously. All calls are guarded: on a runtime without the
 * Samsung APIs they report "not available" instead of throwing.
 */
class CTizenWebApis
{
public:
  static TizenNetworkInfo GetNetworkInfo();
  static WasmDeviceInfo GetDeviceInfo();

  /*!
   * \brief Allow or suppress the TV screensaver while this application runs
   * (webapis.appcommon.setScreenSaver).
   * \return false when the runtime has no AppCommon API.
   */
  static bool SetScreenSaverEnabled(bool enabled);
};

} // namespace KODI::PLATFORM::WASM
