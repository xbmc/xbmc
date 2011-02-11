/*
 *      Copyright (C) 2005-2011 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUIWindowSettingsBluetooth.h"
#include "Application.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIWindowManager.h"
#include "FileItem.h"
#include "Util.h"
#include "Settings.h"
#include "guilib/LocalizeStrings.h"
#include "bluetooth/BluetoothManager.h"
#include "GUIUserMessages.h"

using namespace XFILE;

#define CONTROL_BLUETOOTH 2
#define CONTROL_LASTLOADED_DEVICE 3
#define CONTROL_BLUETOOTHAUDIO 4

CGUIWindowSettingsBluetooth::CGUIWindowSettingsBluetooth(void)
    : CGUIWindow(WINDOW_SETTINGS_BLUETOOTH, "SettingsBluetooth.xml")
{
  m_listItems = new CFileItemList; 
  m_active = false;
}

CGUIWindowSettingsBluetooth::~CGUIWindowSettingsBluetooth(void)
{
  delete m_listItems; 
}

bool CGUIWindowSettingsBluetooth::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_PREVIOUS_MENU || action.GetID() == ACTION_PARENT_DIR)
  {
    g_windowManager.PreviousWindow();
    return true;
  }

  return CGUIWindow::OnAction(action);
}

int CGUIWindowSettingsBluetooth::GetSelectedItem()
{
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_BLUETOOTH);
  g_windowManager.SendMessage(msg);

  return msg.GetParam1();
}

void CGUIWindowSettingsBluetooth::OnItemSelected(int iItem)
{
  CFileItemPtr item = m_listItems->Get(iItem);
  if (item == NULL)
    return;

  const char *id = item->GetProperty("id");
  int choice = 0;
  boost::shared_ptr<IBluetoothDevice> device;
  if (id != NULL && id[0] != 0)
  {
    device = g_bluetoothManager.GetDevice(id);
    if (device != NULL)
    {
      // popup the context menu
      CContextButtons choices;
      if (!device->IsConnected())
        choices.Add(2, 16503); // Connect
      else
        choices.Add(3, 16504); // Disconnect
      choices.Add(4, 1210); // Remove

      choice = CGUIDialogContextMenu::ShowAndGetChoice(choices);
    }
  }
  else
  {
    if (CGUIDialogYesNo::ShowAndGetInput(g_localizeStrings.Get(16505), item->GetLabel(), item->GetLabel2(), ""))
      choice = 1; // Pair
  }

  switch (choice)
  {
  case 1:
    g_bluetoothManager.CreateDevice(item->m_strPath);
    break;

  case 2:
    g_bluetoothManager.ConnectDevice(id);
    break;

  case 3:
    g_bluetoothManager.DisconnectDevice(id);
    break;

  case 4:
    g_bluetoothManager.RemoveDevice(id);
    break;
  }
}

bool CGUIWindowSettingsBluetooth::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      CGUIWindow::OnMessage(message);
      m_active = false;
      ClearListItems();
      g_bluetoothManager.StopDiscovery();
      return true;
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BLUETOOTH)
      {
        int iAction = message.GetParam1();
        if (
          iAction == ACTION_SELECT_ITEM ||
          iAction == ACTION_MOUSE_LEFT_CLICK ||
          iAction == ACTION_CONTEXT_MENU ||
          iAction == ACTION_MOUSE_RIGHT_CLICK
        )
        {
          CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_BLUETOOTH);
          g_windowManager.SendMessage(msg);
          int iItem = msg.GetParam1();
          OnItemSelected(iItem);
          return true;
        }
      }
    }
    break;

  case GUI_MSG_UPDATE_ITEM:
    {
      UpdateDevice((IBluetoothDevice*)message.GetPointer());
    }
    break;

  case GUI_MSG_REMOVE_ITEM:
    {
      if (message.GetParam1() == 0)
        RemoveDiscoveredDevice((const char*)message.GetPointer());
      else
        RemoveDevice((const char*)message.GetPointer());
    }
    break;
  }

  return CGUIWindow::OnMessage(message);
}

void CGUIWindowSettingsBluetooth::UpdateDevice(IBluetoothDevice *device)
{
  if (m_active)
  {
    const char *address = device->GetAddress();
    if (address == NULL || address[0] == 0)
      return;
    bool existing = true;
    CFileItemPtr item = m_listItems->Get(address);
    if (item == NULL)
    {
      item.reset(new CFileItem(address, false));
      m_listItems->Add(item);
      existing = false;
    }
    item->SetLabel(device->GetName() ? device->GetName() : address);
    item->SetLabel2(address);
    switch (device->GetDeviceType())
    {
    case IBluetoothDevice::DEVICE_TYPE_MOUSE:
      item->SetProperty("type", "Mouse");
      break;
    case IBluetoothDevice::DEVICE_TYPE_KEYBOARD:
      item->SetProperty("type", "Keyboard");
      break;
    case IBluetoothDevice::DEVICE_TYPE_HEADSET:
    case IBluetoothDevice::DEVICE_TYPE_HEADPHONES:
    case IBluetoothDevice::DEVICE_TYPE_AUDIO:
      item->SetProperty("type", "Audio");
      break;
    case IBluetoothDevice::DEVICE_TYPE_PHONE:
      item->SetProperty("type", "Phone");
      break;
    case IBluetoothDevice::DEVICE_TYPE_COMPUTER:
      item->SetProperty("type", "Computer");
      break;
    default:
      break;
    }
    if (device->IsCreated())
    {
      item->SetProperty("id", device->GetID());
      item->SetProperty("iscreated", true);
      if (device->IsConnected())
        item->SetProperty("status", "Connected");
      else
        item->SetProperty("status", "Disconnected");
    }
    else if (!existing)
      item->SetProperty("status", "Search");
    RefreshList();
  }
}

void CGUIWindowSettingsBluetooth::RemoveDiscoveredDevice(const char *address)
{
  if (m_active)
  {
    CFileItemPtr item = m_listItems->Get(address);
    if (item != NULL && !item->GetPropertyBOOL("iscreated"))
    {
      m_listItems->Remove(item.get());
      RefreshList();
    }
  }
}

void CGUIWindowSettingsBluetooth::RemoveDevice(const char *id)
{
  if (m_active)
  {
    int i;
    for (i = 0; i < m_listItems->Size(); i++)
    {
      CFileItemPtr item = m_listItems->Get(i);
      if (strcmp(item->GetProperty("id"), id) == 0)
      {
        m_listItems->Remove(i);
        RefreshList();
        break;
      }
    }
  }
}

void CGUIWindowSettingsBluetooth::LoadList()
{
  std::vector<boost::shared_ptr<IBluetoothDevice> > devs;
  int i;

  ClearListItems();

  devs = g_bluetoothManager.GetDevices();
  for (i = 0; i < (int)devs.size(); i++)
  {
    if (devs[i] != NULL)
    {
      UpdateDevice(devs[i].get());
    }
  }
}

void CGUIWindowSettingsBluetooth::ClearListItems()
{
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_BLUETOOTH);
  g_windowManager.SendMessage(msg);

  m_listItems->Clear();
}

void CGUIWindowSettingsBluetooth::RefreshList()
{
  CGUIMessage msg1(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_BLUETOOTH);
  g_windowManager.SendMessage(msg1);
  int iItem = msg1.GetParam1();
  if (iItem >= m_listItems->Size())
    iItem = m_listItems->Size() - 1;
  if (iItem < 0)
    iItem = 0;
  CGUIMessage msg2(GUI_MSG_LABEL_BIND, GetID(), CONTROL_BLUETOOTH, iItem, 0, m_listItems);
  OnMessage(msg2);
}

void CGUIWindowSettingsBluetooth::OnInitWindow()
{
  m_active = true;
  LoadList();
  g_bluetoothManager.StartDiscovery();
  CGUIWindow::OnInitWindow();
}

