/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogPeripherals.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "dialogs/GUIDialogSelect.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "peripherals/Peripherals.h"
#include "peripherals/dialogs/GUIDialogPeripheralSettings.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

#include <mutex>

using namespace KODI;
using namespace PERIPHERALS;

CGUIDialogPeripherals::CGUIDialogPeripherals()
{
  // Initialize CGUIControl via CGUIDialogSelect
  SetID(WINDOW_DIALOG_PERIPHERALS);
}

CGUIDialogPeripherals::~CGUIDialogPeripherals() = default;

void CGUIDialogPeripherals::OnInitWindow()
{
  UpdatePeripheralsSync();
  CGUIDialogSelect::OnInitWindow();
}

void CGUIDialogPeripherals::RegisterPeripheralManager(CPeripherals& manager)
{
  m_manager = &manager;
  m_manager->RegisterObserver(this);
}

void CGUIDialogPeripherals::UnregisterPeripheralManager()
{
  if (m_manager != nullptr)
  {
    m_manager->UnregisterObserver(this);
    m_manager = nullptr;
  }
}

CFileItemPtr CGUIDialogPeripherals::GetItem(unsigned int pos) const
{
  CFileItemPtr item;

  std::unique_lock lock(m_peripheralsMutex);

  if (static_cast<int>(pos) < m_peripherals.Size())
    item = m_peripherals[pos];

  return item;
}

void CGUIDialogPeripherals::Show(CPeripherals& manager)
{
  CGUIDialogPeripherals* pDialog =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogPeripherals>(
          WINDOW_DIALOG_PERIPHERALS);
  if (pDialog == nullptr)
    return;

  pDialog->Reset();

  int iPos = -1;
  do
  {
    pDialog->SetHeading(CVariant{35000});
    pDialog->SetUseDetails(true);

    pDialog->RegisterPeripheralManager(manager);

    pDialog->Open();

    pDialog->UnregisterPeripheralManager();

    iPos = pDialog->IsConfirmed() ? pDialog->GetSelectedItem() : -1;

    if (iPos >= 0)
    {
      CFileItemPtr pItem = pDialog->GetItem(iPos);

      // Show an error if the peripheral doesn't have any settings
      PeripheralPtr peripheral = manager.GetByPath(pItem->GetPath());
      if (!peripheral || !peripheral->HasConfigurableSettings())
      {
        MESSAGING::HELPERS::ShowOKDialogText(CVariant{35000}, CVariant{35004});
        continue;
      }

      CGUIDialogPeripheralSettings* pSettingsDialog =
          CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogPeripheralSettings>(
              WINDOW_DIALOG_PERIPHERAL_SETTINGS);
      if (pItem && pSettingsDialog)
      {
        // Pass peripheral item properties to settings dialog so skin authors
        // Can use it to show more detailed information about the device
        pSettingsDialog->SetProperty("vendor", pItem->GetProperty("vendor"));
        pSettingsDialog->SetProperty("product", pItem->GetProperty("product"));
        pSettingsDialog->SetProperty("bus", pItem->GetProperty("bus"));
        pSettingsDialog->SetProperty("location", pItem->GetProperty("location"));
        pSettingsDialog->SetProperty("class", pItem->GetProperty("class"));
        pSettingsDialog->SetProperty("version", pItem->GetProperty("version"));
        pSettingsDialog->SetProperty("battery", pItem->GetProperty("battery"));

        // Open settings dialog
        pSettingsDialog->SetFileItem(pItem.get());
        pSettingsDialog->RegisterPeripheralManager(manager);
        pSettingsDialog->Open();
        pSettingsDialog->UnregisterPeripheralManager();
      }
    }
  } while (pDialog->IsConfirmed());
}

bool CGUIDialogPeripherals::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_REFRESH_LIST:
    {
      if (m_manager && message.GetControlId() == -1)
      {
        UpdatePeripheralsSync();
        return true;
      }
    }
    default:
      break;
  }

  return CGUIDialogSelect::OnMessage(message);
}

void CGUIDialogPeripherals::FreeResources(bool immediately /* = false */)
{
  // Free GUI resources
  for (auto it = m_peripherals.begin(); it != m_peripherals.end(); ++it)
    (*it)->FreeMemory();

  // Free ancestor resources
  CGUIDialogSelect::FreeResources(immediately);
}

void CGUIDialogPeripherals::Notify(const Observable& obs, const ObservableMessage msg)
{
  switch (msg)
  {
    case ObservableMessagePeripheralsChanged:
      UpdatePeripheralsAsync();
      break;
    default:
      break;
  }
}

void CGUIDialogPeripherals::UpdatePeripheralsAsync()
{
  CGUIMessage msg(GUI_MSG_REFRESH_LIST, GetID(), -1);
  CServiceBroker::GetAppMessenger()->SendGUIMessage(msg);
}

void CGUIDialogPeripherals::UpdatePeripheralsSync()
{
  int iPos = GetSelectedItem();

  std::unique_lock lock(m_peripheralsMutex);

  CFileItemPtr selectedItem;
  if (iPos > 0)
    selectedItem = GetItem(iPos);

  m_peripherals.Clear();
  m_manager->GetDirectory("peripherals://all/", m_peripherals);

  for (const auto& item : m_peripherals)
  {
    const PeripheralPtr peripheral = m_manager->GetByPath(item->GetPath());
    if (!peripheral)
      continue;

    const PeripheralBattery battery = peripheral->GetBattery();
    if (battery.state == PeripheralBatteryState::WIRED)
    {
      // "Wired"
      item->SetLabel2(CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(35013));
    }
    else if (battery.level >= 0)
    {
      // "Battery level"
      item->SetLabel2(StringUtils::Format(
          "{}: {}%", CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(12395),
          battery.level));
      item->SetProperty("battery", battery.level);
    }
  }

  SetItems(m_peripherals);
}
