/*
 *  Copyright (C) 2015-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PeripheralBusApplication.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "XBDateTime.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerIDs.h"
#include "games/controllers/ControllerLayout.h"
#include "games/controllers/ControllerManager.h"
#include "guilib/LocalizeStrings.h"
#include "peripherals/Peripherals.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

using namespace KODI;
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
  {
    PeripheralPtr peripheral =
        m_manager.GetByPath(MakeLocation(PeripheralType::PERIPHERAL_KEYBOARD));
    if (peripheral && peripheral->LastActive().IsValid())
    {
      GAME::ControllerPtr controller = peripheral->ControllerProfile();
      if (!controller)
        controller = CServiceBroker::GetGameControllerManager().GetDefaultKeyboard();

      std::shared_ptr<CFileItem> item = std::make_shared<CFileItem>(peripheral->DeviceName());
      item->SetPath(peripheral->FileLocation());
      item->SetProperty("bus", PeripheralTypeTranslator::BusTypeToString(m_type));
      item->SetProperty("location", peripheral->Location());
      item->SetProperty("class", PeripheralTypeTranslator::TypeToString(peripheral->Type()));
      if (controller)
        item->SetArt("icon", controller->Layout().ImagePath());
      items.Add(item);
    }
  }

  {
    PeripheralPtr peripheral = m_manager.GetByPath(MakeLocation(PeripheralType::PERIPHERAL_MOUSE));
    if (peripheral && peripheral->LastActive().IsValid())
    {
      GAME::ControllerPtr controller = peripheral->ControllerProfile();
      if (!controller)
        controller = CServiceBroker::GetGameControllerManager().GetDefaultMouse();

      std::shared_ptr<CFileItem> item = std::make_shared<CFileItem>(peripheral->DeviceName());
      item->SetPath(peripheral->FileLocation());
      item->SetProperty("bus", PeripheralTypeTranslator::BusTypeToString(m_type));
      item->SetProperty("location", peripheral->Location());
      item->SetProperty("class", PeripheralTypeTranslator::TypeToString(peripheral->Type()));
      if (controller)
        item->SetArt("icon", controller->Layout().ImagePath());
      items.Add(item);
    }
  }
}

std::string CPeripheralBusApplication::MakeLocation(PeripheralType peripheralType)
{
  return StringUtils::Format(
      "peripherals://{}/{}.dev",
      PeripheralTypeTranslator::BusTypeToString(PeripheralBusType::PERIPHERAL_BUS_APPLICATION),
      PeripheralTypeTranslator::TypeToString(peripheralType));
}
