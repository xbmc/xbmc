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
#include "peripherals/Peripherals.h"
#include "peripherals/dialogs/GUIDialogPeripheralSettings.h"
#include "utils/Variant.h"
#include "FileItem.h"

using namespace KODI;
using namespace MESSAGING;
using namespace PERIPHERALS;

CGUIDialogPeripherals::CGUIDialogPeripherals(CPeripherals &manager) :
  m_manager(manager)
{
  m_manager.RegisterObserver(this);
}

CGUIDialogPeripherals::~CGUIDialogPeripherals()
{
  m_manager.UnregisterObserver(this);
}

void CGUIDialogPeripherals::Show(CPeripherals &manager)
{
  CGUIDialogPeripherals dialog(manager);
  dialog.ShowInternal();
}

void CGUIDialogPeripherals::ShowInternal()
{
  CGUIDialogSelect* pDialog = g_windowManager.GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
  if (pDialog == nullptr)
    return;

  CFileItemList items;
  m_manager.GetDirectory("peripherals://all/", items);

  int iPos = -1;
  do
  {
    pDialog->Reset();
    pDialog->SetHeading(CVariant{ 35000 });
    pDialog->SetUseDetails(true);
    pDialog->SetItems(items);
    pDialog->SetSelected(iPos);
    pDialog->Open();

    iPos = pDialog->IsConfirmed() ? pDialog->GetSelectedItem() : -1;

    if (iPos >= 0)
    {
      CFileItemPtr pItem = items.Get(iPos);

      // Show an error if the peripheral doesn't have any settings
      PeripheralPtr peripheral = m_manager.GetByPath(pItem->GetPath());
      if (!peripheral || peripheral->GetSettings().empty())
      {
        HELPERS::ShowOKDialogText(CVariant{ 35000 }, CVariant{ 35004 });
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
  //! @todo
}
