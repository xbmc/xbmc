/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIDialogPeripheralManager.h"
#include "GUIDialogPeripheralSettings.h"
#include "guilib/GUIWindowManager.h"
#include "peripherals/Peripherals.h"
#include "FileItem.h"
#include "guilib/Key.h"
#include "utils/log.h"

#define BUTTON_CLOSE     10
#define BUTTON_SETTINGS  11
#define CONTROL_LIST     20

using namespace std;
using namespace PERIPHERALS;

CGUIDialogPeripheralManager::CGUIDialogPeripheralManager(void) :
    CGUIDialog(WINDOW_DIALOG_PERIPHERAL_MANAGER, "DialogPeripheralManager.xml"),
    m_iSelected(0),
    m_peripheralItems(new CFileItemList)
{
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogPeripheralManager::~CGUIDialogPeripheralManager(void)
{
  delete m_peripheralItems;
}

bool CGUIDialogPeripheralManager::OnAction(const CAction &action)
{
  int iActionId = action.GetID();
  if (GetFocusedControlID() == CONTROL_LIST &&
      (iActionId == ACTION_MOVE_DOWN || iActionId == ACTION_MOVE_UP ||
       iActionId == ACTION_PAGE_DOWN || iActionId == ACTION_PAGE_UP))
  {
    CGUIDialog::OnAction(action);
    int iSelected = m_viewControl.GetSelectedItem();
    if (iSelected != m_iSelected)
      m_iSelected = iSelected;
    UpdateButtons();
    return true;
  }

  return CGUIDialog::OnAction(action);
}

void CGUIDialogPeripheralManager::OnInitWindow()
{
  CGUIWindow::OnInitWindow();
  m_iSelected = 0;
  Update();
}

bool CGUIDialogPeripheralManager::OnClickList(CGUIMessage &message)
{
  if (CurrentItemHasSettings())
    return OpenSettingsDialog();

  return true;
}

bool CGUIDialogPeripheralManager::OnClickButtonClose(CGUIMessage &message)
{
  Close();
  return true;
}

bool CGUIDialogPeripheralManager::OnClickButtonSettings(CGUIMessage &message)
{
  return OpenSettingsDialog();
}

bool CGUIDialogPeripheralManager::OpenSettingsDialog(void)
{
  CGUIDialogPeripheralSettings *dialog = (CGUIDialogPeripheralSettings *)g_windowManager.GetWindow(WINDOW_DIALOG_PERIPHERAL_SETTINGS);
  if (dialog)
  {
    dialog->SetFileItem(GetCurrentListItem());
    dialog->DoModal();
    return true;
  }

  return false;
}

bool CGUIDialogPeripheralManager::OnMessageClick(CGUIMessage &message)
{
  int iControl = message.GetSenderId();
  switch(iControl)
  {
  case CONTROL_LIST:
    return OnClickList(message);
  case BUTTON_CLOSE:
    return OnClickButtonClose(message);
  case BUTTON_SETTINGS:
    return OnClickButtonSettings(message);
  default:
    return false;
  }
}

bool CGUIDialogPeripheralManager::OnMessage(CGUIMessage& message)
{
  unsigned int iMessage = message.GetMessage();

  switch (iMessage)
  {
    case GUI_MSG_WINDOW_DEINIT:
      Clear();
      break;
    case GUI_MSG_ITEM_SELECT:
      return true;
    case GUI_MSG_CLICKED:
      return OnMessageClick(message);
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogPeripheralManager::OnWindowLoaded(void)
{
  CGUIDialog::OnWindowLoaded();

  m_viewControl.Reset();
  m_viewControl.SetParentWindow(GetID());
  const CGUIControl *list = GetControl(CONTROL_LIST);
  m_viewControl.AddView(list);
}

void CGUIDialogPeripheralManager::OnWindowUnload(void)
{
  CGUIDialog::OnWindowUnload();
  m_viewControl.Reset();
}

CFileItemPtr CGUIDialogPeripheralManager::GetCurrentListItem(void) const
{
  return m_peripheralItems->Get(m_iSelected);
}

void CGUIDialogPeripheralManager::Update()
{
  CSingleLock lock(g_graphicsContext);

  m_viewControl.SetCurrentView(CONTROL_LIST);
  Clear();
  g_peripherals.GetDirectory("peripherals://all/", *m_peripheralItems);
  m_viewControl.SetItems(*m_peripheralItems);
  m_viewControl.SetSelectedItem(m_iSelected);

  UpdateButtons();
  CGUIControl *list = (CGUIControl *) GetControl(CONTROL_LIST);
  if (list)
    list->SetInvalid();
}

void CGUIDialogPeripheralManager::Clear(void)
{
  m_viewControl.Clear();
  m_peripheralItems->Clear();
}

bool CGUIDialogPeripheralManager::CurrentItemHasSettings(void) const
{
  CSingleLock lock(g_graphicsContext);
  CFileItemPtr currentItem = GetCurrentListItem();
  if (!currentItem)
    return false;

  CPeripheral *peripheral = g_peripherals.GetByPath(currentItem.get()->GetPath());
  return peripheral && peripheral->HasConfigurableSettings();
}

void CGUIDialogPeripheralManager::UpdateButtons(void)
{
  CONTROL_ENABLE_ON_CONDITION(BUTTON_SETTINGS, CurrentItemHasSettings());
}
