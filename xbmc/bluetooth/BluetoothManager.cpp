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

#include "system.h"
#include "Application.h"
#include "guilib/GUIWindowManager.h"
#include "BluetoothManager.h"
#include "utils/log.h"
#include "guilib/LocalizeStrings.h"
#include "GUIUserMessages.h"

#if defined(_LINUX) && defined(HAS_DBUS)
#include "linux/BluezBluetoothSyscall.h"
#endif

CBluetoothManager g_bluetoothManager;

CBluetoothManager::CBluetoothManager()
{
  m_instance = NULL;
}

CBluetoothManager::~CBluetoothManager()
{
  delete m_instance;
}

void CBluetoothManager::Initialize()
{
#if defined(_LINUX) && defined(HAS_DBUS)
  m_instance = new CBluezBluetoothSyscall();
#endif

  if (m_instance == NULL)
    m_instance = new CNullBluetoothSyscall();
}

std::vector<boost::shared_ptr<IBluetoothDevice> > CBluetoothManager::GetDevices()
{
  return m_instance->GetDevices();
}

boost::shared_ptr<IBluetoothDevice> CBluetoothManager::GetDevice(const char *id)
{
  return m_instance->GetDevice(id);
}

void CBluetoothManager::StartDiscovery()
{
  m_instance->StartDiscovery();
}

void CBluetoothManager::StopDiscovery()
{
  m_instance->StopDiscovery();
}

void CBluetoothManager::CreateDevice(const char *address)
{
  m_instance->CreateDevice(address);
}

void CBluetoothManager::RemoveDevice(const char *id)
{
  m_instance->RemoveDevice(id);
}

void CBluetoothManager::ConnectDevice(const char *id)
{
  m_instance->ConnectDevice(id);
}

void CBluetoothManager::DisconnectDevice(const char *id)
{
  m_instance->DisconnectDevice(id);
}

void CBluetoothManager::ProcessEvents()
{
  m_instance->PumpBluetoothEvents(this);
}

void CBluetoothManager::OnDeviceConnected(IBluetoothDevice *device)
{
  if (device->IsPaired())
    g_application.m_guiDialogKaiToast.QueueNotification("Bluetooth.png", device->GetName(), g_localizeStrings.Get(13296));
  CGUIMessage msg(GUI_MSG_UPDATE_ITEM, WINDOW_SETTINGS_BLUETOOTH, 0);
  msg.SetPointer(device);
  g_windowManager.SendMessage(msg);
}

void CBluetoothManager::OnDeviceDisconnected(IBluetoothDevice *device)
{
  if (device->IsPaired())
    g_application.m_guiDialogKaiToast.QueueNotification("Bluetooth.png", device->GetName(), g_localizeStrings.Get(16509));
  CGUIMessage msg(GUI_MSG_UPDATE_ITEM, WINDOW_SETTINGS_BLUETOOTH, 0);
  msg.SetPointer(device);
  g_windowManager.SendMessage(msg);
}

void CBluetoothManager::OnDeviceFound(IBluetoothDevice *device)
{
  CGUIMessage msg(GUI_MSG_UPDATE_ITEM, WINDOW_SETTINGS_BLUETOOTH, 0);
  msg.SetPointer(device);
  g_windowManager.SendMessage(msg);
}

void CBluetoothManager::OnDeviceDisappeared(const char *address)
{
  // Remove by hardware address
  CGUIMessage msg(GUI_MSG_REMOVE_ITEM, WINDOW_SETTINGS_BLUETOOTH, 0, 0);
  msg.SetPointer((void*)address);
  g_windowManager.SendMessage(msg);
}

void CBluetoothManager::OnDeviceCreated(IBluetoothDevice *device)
{
  CGUIMessage msg(GUI_MSG_UPDATE_ITEM, WINDOW_SETTINGS_BLUETOOTH, 0);
  msg.SetPointer(device);
  g_windowManager.SendMessage(msg);
}

void CBluetoothManager::OnDeviceRemoved(const char *id)
{
  // Remove by device id
  CGUIMessage msg(GUI_MSG_REMOVE_ITEM, WINDOW_SETTINGS_BLUETOOTH, 0, 1);
  msg.SetPointer((void*)id);
  g_windowManager.SendMessage(msg);
}

