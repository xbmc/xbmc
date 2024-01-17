/*
 *  Copyright (C) 2015-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PeripheralBusApplication.h"

#include "ServiceBroker.h"
#include "guilib/LocalizeStrings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"

using namespace PERIPHERALS;

CPeripheralBusApplication::CPeripheralBusApplication(CPeripherals& manager)
  : CPeripheralBus("PeripBusApplication", manager, PERIPHERAL_BUS_APPLICATION)
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
    result.m_type = PERIPHERAL_KEYBOARD;
    result.m_strDeviceName = g_localizeStrings.Get(35150); // "Keyboard"
    result.m_strLocation = PeripheralTypeTranslator::TypeToString(PERIPHERAL_KEYBOARD);
    result.m_iVendorId = 0;
    result.m_iProductId = 0;
    result.m_mappedType = PERIPHERAL_KEYBOARD;
    result.m_mappedBusType = Type();
    result.m_iSequence = 0;

    if (!results.ContainsResult(result))
      results.m_results.push_back(result);
  }

  bool bHasMouse = CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
      CSettings::SETTING_INPUT_ENABLEMOUSE);

  //! @todo Fix game clients to handle mouse disconnecting
  //! For now mouse is always connected
  bHasMouse = true;

  if (bHasMouse)
  {
    PeripheralScanResult result(Type());
    result.m_type = PERIPHERAL_MOUSE;
    result.m_strDeviceName = g_localizeStrings.Get(35171); // "Mouse"
    result.m_strLocation = PeripheralTypeTranslator::TypeToString(PERIPHERAL_MOUSE);
    result.m_iVendorId = 0;
    result.m_iProductId = 0;
    result.m_mappedType = PERIPHERAL_MOUSE;
    result.m_mappedBusType = Type();
    result.m_iSequence = 0;

    if (!results.ContainsResult(result))
      results.m_results.push_back(result);
  }

  return true;
}

void CPeripheralBusApplication::GetDirectory(const std::string& strPath, CFileItemList& items) const
{
  // Don't list virtual devices in the GUI
}

std::string CPeripheralBusApplication::MakeLocation(unsigned int controllerIndex) const
{
  return std::to_string(controllerIndex);
}
