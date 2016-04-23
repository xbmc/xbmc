/*
 *      Copyright (C) 2014-2016 Team Kodi
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

#include "Addon_Peripheral.h"

#include "addons/binary/ExceptionHandling.h"
#include "addons/binary/interfaces/api2/AddonInterfaceBase.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api2/.internal/AddonLib_internal.hpp"
#include "peripherals/Peripherals.h"
#include "peripherals/addons/PeripheralAddon.h"
#include "peripherals/devices/Peripheral.h"

using namespace ADDON;
using namespace PERIPHERALS;

namespace V2
{
namespace KodiAPI
{

namespace Peripheral
{
extern "C"
{

void CAddOnPeripheral::Init(struct CB_AddOnLib *interfaces)
{
  interfaces->Peripheral.trigger_scan        = V2::KodiAPI::Peripheral::CAddOnPeripheral::trigger_scan;
  interfaces->Peripheral.refresh_button_maps = V2::KodiAPI::Peripheral::CAddOnPeripheral::refresh_button_maps;
}

CPeripheralAddon* CAddOnPeripheral::GetPeripheralAddon(void* hdl, const char* strFunction)
{
  CAddonInterfaces* addon = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);
  if (!addon || !addon->AddOnLib_GetHelper())
    throw ADDON::WrongValueException("CAddOnPeripheral - %s - called with a null pointer", strFunction);

  return dynamic_cast<CPeripheralAddon*>(static_cast<CAddonInterfaceAddon*>(addon->AddOnLib_GetHelper())->GetAddon());
}

void CAddOnPeripheral::trigger_scan(void* hdl)
{
  try
  {
    g_peripherals.TriggerDeviceScan(PERIPHERAL_BUS_ADDON);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnPeripheral::refresh_button_maps(void* hdl, const char* deviceName, const char* controllerId)
{
  try
  {
    if (!hdl)
      throw ADDON::WrongValueException("CAddOnPeripheral - %s - invalid data (handle='%p')",
                                        __FUNCTION__, hdl);

    CPeripheralAddon* peripheralAddon = GetPeripheralAddon(hdl, __FUNCTION__);
    if (!peripheralAddon)
      return;

    peripheralAddon->RefreshButtonMaps(deviceName ? deviceName : "", controllerId ? controllerId : "");
  }
  HANDLE_ADDON_EXCEPTION
}

} /* extern "C" */
} /* namespace Peripheral */

} /* namespace KodiAPI */
} /* namespace V2 */
