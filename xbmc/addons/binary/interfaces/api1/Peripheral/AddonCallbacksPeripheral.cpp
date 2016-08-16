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

#include "AddonCallbacksPeripheral.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerLayout.h"
#include "peripherals/Peripherals.h"
#include "peripherals/addons/PeripheralAddon.h"
#include "peripherals/addons/PeripheralAddonTranslator.h"
#include "utils/log.h"

using namespace ADDON;
using namespace PERIPHERALS;

namespace V1
{
namespace KodiAPI
{

namespace Peripheral
{

CAddonCallbacksPeripheral::CAddonCallbacksPeripheral(ADDON::CAddon* addon)
  : ADDON::IAddonInterface(addon, 1, PERIPHERAL_API_VERSION),
    m_callbacks(new CB_PeripheralLib)
{
  /* write Kodi peripheral specific add-on function addresses to callback table */
  m_callbacks->TriggerScan               = TriggerScan;
  m_callbacks->RefreshButtonMaps         = RefreshButtonMaps;
  m_callbacks->FeatureCount              = FeatureCount;
}

CAddonCallbacksPeripheral::~CAddonCallbacksPeripheral()
{
  /* delete the callback table */
  delete m_callbacks;
}

CPeripheralAddon* CAddonCallbacksPeripheral::GetPeripheralAddon(void* addonData, const char* strFunction)
{
  CAddonInterfaces* addon = static_cast<CAddonInterfaces*>(addonData);
  if (!addon || !addon->GetHelperPeripheral())
  {
    CLog::Log(LOGERROR, "PERIPHERAL - %s - called with a null pointer", strFunction);
    return NULL;
  }

  return dynamic_cast<CPeripheralAddon*>(static_cast<CAddonCallbacksPeripheral*>(addon->GetHelperPeripheral())->m_addon);
}

void CAddonCallbacksPeripheral::TriggerScan(void* addonData)
{
  g_peripherals.TriggerDeviceScan(PERIPHERAL_BUS_ADDON);
}

void CAddonCallbacksPeripheral::RefreshButtonMaps(void* addonData, const char* deviceName, const char* controllerId)
{
  CPeripheralAddon* peripheralAddon = GetPeripheralAddon(addonData, __FUNCTION__);
  if (!peripheralAddon)
    return;

  peripheralAddon->RefreshButtonMaps(deviceName ? deviceName : "", controllerId ? controllerId : "");
}

unsigned int CAddonCallbacksPeripheral::FeatureCount(void* addonData, const char* controllerId, JOYSTICK_FEATURE_TYPE type)
{
  using namespace ADDON;
  using namespace GAME;

  unsigned int count = 0;

  AddonPtr addon;
  if (CAddonMgr::GetInstance().GetAddon(controllerId, addon, ADDON_GAME_CONTROLLER))
  {
    ControllerPtr controller = std::static_pointer_cast<CController>(addon);
    if (controller->LoadLayout())
      count = controller->Layout().FeatureCount(CPeripheralAddonTranslator::TranslateFeatureType(type));
  }

  return count;
}

} /* namespace Peripheral */

} /* namespace KodiAPI */
} /* namespace V1 */
