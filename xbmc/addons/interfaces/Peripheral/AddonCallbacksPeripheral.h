/*
 *      Copyright (C) 2014-2017 Team Kodi
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "addons/interfaces/AddonInterfaces.h"
#include "addons/kodi-addon-dev-kit/include/kodi/libKODI_peripheral.h"

namespace PERIPHERALS { class CPeripheralAddon; }

namespace KodiAPI
{
namespace Peripheral
{

/*!
 * Callbacks for a peripheral add-on to Kodi
 */
class CAddonCallbacksPeripheral
{
public:
  CAddonCallbacksPeripheral(ADDON::CAddon* addon);
  ~CAddonCallbacksPeripheral(void);

  /*!
   * @return The callback table
   */
  CB_PeripheralLib* GetCallbacks() const { return m_callbacks; }

  static void TriggerScan(void* addonData);
  static void RefreshButtonMaps(void* addonData, const char* deviceName, const char* controllerId);
  static unsigned int FeatureCount(void* addonData, const char* controllerId, JOYSTICK_FEATURE_TYPE type);

private:
  static PERIPHERALS::CPeripheralAddon* GetPeripheralAddon(void* addonData, const char* strFunction);

  ADDON::CAddon* m_addon; /*!< the addon */
  CB_PeripheralLib*  m_callbacks; /*!< callback addresses */
};

} /* namespace Peripheral */
} /* namespace KodiAPI */
