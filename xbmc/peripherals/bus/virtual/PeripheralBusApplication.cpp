/*
 *      Copyright (C) 2015-2017 Team Kodi
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

#include "PeripheralBusApplication.h"
#include "ServiceBroker.h"
#include "guilib/LocalizeStrings.h"
#include "settings/Settings.h"
#include "utils/StringUtils.h"

using namespace PERIPHERALS;

CPeripheralBusApplication::CPeripheralBusApplication(CPeripherals& manager) :
    CPeripheralBus("PeripBusApplication", manager, PERIPHERAL_BUS_APPLICATION)
{
  // Initialize CPeripheralBus
  m_bNeedsPolling = false;
}

void CPeripheralBusApplication::Initialise(void)
{
  CPeripheralBus::Initialise();
  TriggerDeviceScan();
}

bool CPeripheralBusApplication::PerformDeviceScan(PeripheralScanResults& results)
{
  {
    PeripheralScanResult result(Type());
    result.m_type          = PERIPHERAL_KEYBOARD;
    result.m_strDeviceName = g_localizeStrings.Get(35150); // "Keyboard"
    result.m_strLocation   = PeripheralTypeTranslator::TypeToString(PERIPHERAL_KEYBOARD);
    result.m_iVendorId     = 0;
    result.m_iProductId    = 0;
    result.m_mappedType    = PERIPHERAL_KEYBOARD;
    result.m_mappedBusType = Type();
    result.m_iSequence     = 0;

    if (!results.ContainsResult(result))
      results.m_results.push_back(result);
  }

  bool bHasMouse = CServiceBroker::GetSettings().GetBool(CSettings::SETTING_INPUT_ENABLEMOUSE);

  //! @todo Fix game clients to handle mouse disconnecting
  //! For now mouse is always connected
  bHasMouse = true;

  if (bHasMouse)
  {
    PeripheralScanResult result(Type());
    result.m_type          = PERIPHERAL_MOUSE;
    result.m_strDeviceName = g_localizeStrings.Get(35171); // "Mouse"
    result.m_strLocation   = PeripheralTypeTranslator::TypeToString(PERIPHERAL_MOUSE);
    result.m_iVendorId     = 0;
    result.m_iProductId    = 0;
    result.m_mappedType    = PERIPHERAL_MOUSE;
    result.m_mappedBusType = Type();
    result.m_iSequence     = 0;

    if (!results.ContainsResult(result))
      results.m_results.push_back(result);
  }

  return true;
}

void CPeripheralBusApplication::GetDirectory(const std::string &strPath, CFileItemList &items) const
{
  // Don't list virtual devices in the GUI
}

std::string CPeripheralBusApplication::MakeLocation(unsigned int controllerIndex) const
{
  return StringUtils::Format("%u", controllerIndex);
}
