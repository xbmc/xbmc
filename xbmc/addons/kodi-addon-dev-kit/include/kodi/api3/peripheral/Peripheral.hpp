#pragma once
/*
 *      Copyright (C) 2015-2016 Team KODI
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "../definitions.hpp"

API_NAMESPACE

namespace KodiAPI
{
namespace Peripheral
{

  //============================================================================
  ///
  /// @ingroup CPP_KodiAPI_Peripheral
  /// @brief Trigger a scan for peripherals
  ///
  /// The add-on calls this if a change in hardware is detected.
  ///
  void TriggerScan(void);

  //============================================================================
  ///
  /// @ingroup CPP_KodiAPI_Peripheral
  /// @brief Notify the frontend that button maps have changed
  ///
  /// @param[in] deviceName       [optional] The name of the device to  refresh,
  ///                             or empty/null for all devices
  /// @param[in] controllerId     [optional] The  controller ID to  refresh,  or
  ///                             empty/null for all controllers
  ///
  void RefreshButtonMaps(const std::string& deviceName = "", const std::string& controllerId = "");

} /* namespace Peripheral */
} /* namespace KodiAPI */

END_NAMESPACE()
