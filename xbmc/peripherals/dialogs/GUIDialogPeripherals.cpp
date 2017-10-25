/*
 *      Copyright (C) 2017 Team Kodi
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

#include "GUIDialogPeripherals.h"
#include "dialogs/GUIDialogSelect.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "messaging/ApplicationMessenger.h"
#include "peripherals/Peripherals.h"
#include "peripherals/dialogs/GUIDialogPeripheralSettings.h"
#include "threads/SingleLock.h"
#include "utils/Variant.h"
#include "FileItem.h"

using namespace KODI;
using namespace PERIPHERALS;

CGUIDialogPeripherals::CGUIDialogPeripherals()
{
  // Initialize CGUIControl via CGUIDialogSelect
  SetID(WINDOW_DIALOG_PERIPHERALS);
}

CGUIDialogPeripherals::~CGUIDialogPeripherals() = default;

void CGUIDialogPeripherals::RegisterPeripheralManager(CPeripherals &manager)
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

  CSingleLock lock(m_peripheralsMutex);

  if (static_cast<int>(pos) < m_peripherals.Size())
    item = m_peripherals[pos];

  return item;
}

void CGUIDialogPeripherals::Show(CPeripherals &manager)
{
  CGUIDialogPeripherals* pDialog = g_windowManager.GetWindow<CGUIDialogPeripherals>(WINDOW_DIALOG_PERIPHERALS);
  if (pDialog == nullptr)
    return;

  int iPos = -1;
  do
  {
    pDialog->Reset();
    pDialog->SetHeading(CVariant{ 35000 });
    pDialog->SetUseDetails(true);

    pDialog->RegisterPeripheralManager(manager);
    pDialog->UpdatePeripherals();

    pDialog->Open();

    pDialog->UnregisterPeripheralManager();

    iPos = pDialog->IsConfirmed() ? pDialog->GetSelectedItem() : -1;

    if (iPos >= 0)
    {
      CFileItemPtr pItem = pDialog->GetItem(iPos);

      // Show an error if the peripheral doesn't have any settings
      PeripheralPtr peripheral = manager.GetByPath(pItem->GetPath());
      if (!peripheral || peripheral->GetSettings().empty())
      {
        MESSAGING::HELPERS::ShowOKDialogText(CVariant{ 35000 }, CVariant{ 35004 });
        continue;
      }

      CGUIDialogPeripheralSettings *pSettingsDialog = g_windowManager.GetWindow<CGUIDialogPeripheralSettings>(WINDOW_DIALOG_PERIPHERAL_SETTINGS);
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

        // Open settings dialog
        pSettingsDialog->SetFileItem(pItem.get());
        pSettingsDialog->Open();
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
        CSingleLock lock(m_peripheralsMutex);
        m_peripherals.Clear();
        m_manager->GetDirectory("peripherals://all/", m_peripherals);
        SetItems(m_peripherals);
      }
      return true;
    }
    default:
      break;
  }

  return CGUIDialogSelect::OnMessage(message);
}

void CGUIDialogPeripherals::Notify(const Observable &obs, const ObservableMessage msg)
{
  switch (msg)
  {
  case ObservableMessagePeripheralsChanged:
    UpdatePeripherals();
    break;
  default:
    break;
  }
}

void CGUIDialogPeripherals::UpdatePeripherals()
{
  CGUIMessage msg(GUI_MSG_REFRESH_LIST, GetID(), -1);
  MESSAGING::CApplicationMessenger::GetInstance().SendGUIMessage(msg);
}
