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
#include "dialogs/GUIDialogKeyboard.h"
#include "dialogs/GUIDialogNumeric.h"
#include "dialogs/GUIDialogOK.h"
#include "guilib/GUIWindowManager.h"
#include "utils/log.h"
#include "guilib/LocalizeStrings.h"

#ifdef HAS_DBUS
#include "DBusUtil.h"
#include <string>
#include "BluezBluetoothSyscall.h"

/**************** CBluezBluetoothDevice ***************/

CBluezBluetoothDevice::CBluezBluetoothDevice(const char* id)
{
  CVariant properties = CDBusUtil::GetAll("org.bluez", id, "org.bluez.Device", "GetProperties", NULL);
  m_id = id;
  m_name = properties["Name"].asString();
  m_address = properties["Address"].asString();
  m_connected = properties["Connected"].asBoolean();
  m_created = true;
  m_paired = properties["Paired"].asBoolean();
  m_deviceType = GetDeviceTypeFromClass((uint32_t)properties["Class"].asUnsignedInteger());
}

CBluezBluetoothDevice::CBluezBluetoothDevice(const char* address, const char *name, uint32_t cls)
{
  m_id = "";
  m_name = name;
  m_address = address;
  m_connected = false;
  m_created = false;
  m_paired = false;
  m_deviceType = GetDeviceTypeFromClass(cls);
}

IBluetoothDevice::DeviceType CBluezBluetoothDevice::GetDeviceTypeFromClass(uint32_t cls)
{
  switch ((cls & 0x1f00) >> 8)
  {
  case 0x01:
    return DEVICE_TYPE_COMPUTER;
  case 0x02:
    switch ((cls & 0xfc) >> 2)
    {
    case 0x01:
    case 0x02:
    case 0x03:
    case 0x05:
      return DEVICE_TYPE_PHONE;
    case 0x04:
      return DEVICE_TYPE_MODEM;
    }
    break;
  case 0x03:
    return DEVICE_TYPE_NETWORK;
  case 0x04:
    switch ((cls & 0xfc) >> 2)
    {
    case 0x01:
    case 0x02:
      return DEVICE_TYPE_HEADSET;
    case 0x06:
      return DEVICE_TYPE_HEADPHONES;
    case 0x0b:
    case 0x0c:
    case 0x0d:
      return DEVICE_TYPE_VIDEO;
    default:
      return DEVICE_TYPE_AUDIO;
    }
    break;
  case 0x05:
    switch ((cls & 0xc0) >> 6)
    {
    case 0x00:
      switch ((cls & 0x1e) >> 2)
      {
      case 0x01:
      case 0x02:
        return DEVICE_TYPE_JOYPAD;
      }
      break;
    case 0x01:
      return DEVICE_TYPE_KEYBOARD;
    case 0x02:
      switch ((cls & 0x1e) >> 2)
      {
      case 0x05:
        return DEVICE_TYPE_TABLET;
      default:
        return DEVICE_TYPE_MOUSE;
      }
    }
    break;
  case 0x06:
    if (cls & 0x80)
      return DEVICE_TYPE_PRINTER;
    if (cls & 0x20)
      return DEVICE_TYPE_CAMERA;
    break;
  }

  return DEVICE_TYPE_UNKNOWN;
}

/**************** CBluezBluetoothAgent ***************/

CBluezBluetoothAgent::CBluezBluetoothAgent(DBusConnection *conn)
  : CDBusAgent<CBluezBluetoothAgent>(conn, "/org/xbmc/BluetoothAgent")
{
  RegisterMethod("org.bluez.Agent", "Release", &CBluezBluetoothAgent::Release);
  RegisterMethod("org.bluez.Agent", "RequestPinCode", &CBluezBluetoothAgent::RequestPinCode);
  RegisterMethod("org.bluez.Agent", "RequestPasskey", &CBluezBluetoothAgent::RequestPasskey);
  RegisterMethod("org.bluez.Agent", "DisplayPasskey", &CBluezBluetoothAgent::DisplayPasskey);
  RegisterMethod("org.bluez.Agent", "RequestConfirmation", &CBluezBluetoothAgent::RequestConfirmation);
  RegisterMethod("org.bluez.Agent", "Authorize", &CBluezBluetoothAgent::Authorize);
  RegisterMethod("org.bluez.Agent", "ConfirmModeChange", &CBluezBluetoothAgent::ConfirmModeChange);
  RegisterMethod("org.bluez.Agent", "Cancel", &CBluezBluetoothAgent::Cancel);
}

void CBluezBluetoothAgent::Release(std::vector<CVariant> &args)
{
  Reply();
}

void CBluezBluetoothAgent::RequestPinCode(std::vector<CVariant> &args)
{
  CBluezBluetoothDevice device(args[0].asString());
  CStdString key;
  switch (device.GetDeviceType())
  {
  case IBluetoothDevice::DEVICE_TYPE_KEYBOARD:
    key.Format("%06u", rand() % 1000000);
    Reply(key);
    CGUIDialogOK::ShowAndGetInput(key, 16507, 16508, "");
    break;
  case IBluetoothDevice::DEVICE_TYPE_COMPUTER:
  case IBluetoothDevice::DEVICE_TYPE_PHONE:
    key.Format("%06u", rand() % 1000000);
    Reply(key);
    CGUIDialogOK::ShowAndGetInput(key, 16507, "", "");
    break;
  default:
    if (CGUIDialogKeyboard::ShowAndGetInput(key, g_localizeStrings.Get(16506), true))
      Reply(key);
    else
      ReplyError("org.bluez.Error.Canceled", "User canceled.");
    break;
  }
}

void CBluezBluetoothAgent::RequestPasskey(std::vector<CVariant> &args)
{
  CStdString key;
  if (CGUIDialogNumeric::ShowAndVerifyInput(key, g_localizeStrings.Get(16506), false))
    Reply((unsigned int)atoi(key));
  else
    ReplyError("org.bluez.Error.Canceled", "User canceled.");
}

void CBluezBluetoothAgent::DisplayPasskey(std::vector<CVariant> &args)
{
  Reply();
  CGUIDialogOK *dialog = (CGUIDialogOK *)g_windowManager.GetWindow(WINDOW_DIALOG_OK);
  if (!dialog)
    return;
  dialog->Close();
  CGUIDialogOK::ShowAndGetInput(16507, args[1], "", "");
}

void CBluezBluetoothAgent::RequestConfirmation(std::vector<CVariant> &args)
{
  Reply();
}

void CBluezBluetoothAgent::Authorize(std::vector<CVariant> &args)
{
  Reply();
}

void CBluezBluetoothAgent::ConfirmModeChange(std::vector<CVariant> &args)
{
  Reply();
}

void CBluezBluetoothAgent::Cancel(std::vector<CVariant> &args)
{
  Reply();
  CGUIDialogOK *dialog = (CGUIDialogOK *)g_windowManager.GetWindow(WINDOW_DIALOG_OK);
  if (!dialog)
    return;
  dialog->Close();
}

/**************** CBluezBluetoothSyscall ***************/

CBluezBluetoothSyscall::CBluezBluetoothSyscall()
{
  SetupDefaultAdapter();

  m_agent = NULL;

  if (m_hasAdapter)
  {
    DBusError error;

    dbus_error_init (&error);
    m_connection = dbus_bus_get_private(DBUS_BUS_SYSTEM, &error);

    if (m_connection)
    {
      dbus_connection_set_exit_on_disconnect(m_connection, false);

      dbus_bus_add_match(m_connection, "type='signal',interface='org.bluez.Adapter'", &error);
      if (!dbus_error_is_set(&error))
      {
        dbus_bus_add_match(m_connection, "type='signal',interface='org.bluez.Device'", &error);
      }
      dbus_connection_flush(m_connection);
    }

    if (dbus_error_is_set(&error))
    {
      CLog::Log(LOGERROR, "Bluetooth: Failed to attach to signal %s", error.message);
      dbus_connection_close(m_connection);
      dbus_connection_unref(m_connection);
      m_connection = NULL;
    }
    dbus_error_free (&error);
  }
}

CBluezBluetoothSyscall::~CBluezBluetoothSyscall()
{
  if (m_connection)
  {
    dbus_connection_close(m_connection);
    dbus_connection_unref(m_connection);
    m_connection = NULL;
  }
}

std::vector<boost::shared_ptr<IBluetoothDevice> > CBluezBluetoothSyscall::GetDevices()
{
  std::vector<boost::shared_ptr<IBluetoothDevice> > v;

  if (m_hasAdapter)
  {
    CVariant properties = CDBusUtil::GetAll("org.bluez", m_adapter.c_str(), "org.bluez.Adapter", "GetProperties", NULL);
    CVariant devices = properties["Devices"];
    unsigned int i;
    for (i = 0; i < devices.size(); i++)
    {
      CLog::Log(LOGDEBUG, "Bluetooth: found known device %s", devices[i].asString());
      v.push_back(GetDevice(devices[i].asString()));
    }
  }
  return v;
}

boost::shared_ptr<IBluetoothDevice> CBluezBluetoothSyscall::GetDevice(const char *id)
{
  boost::shared_ptr<IBluetoothDevice> p(new CBluezBluetoothDevice(id));
  return p;
}

void CBluezBluetoothSyscall::SetupDefaultAdapter()
{
  CDBusMessage message("org.bluez", "/", "org.bluez.Manager", "DefaultAdapter");
  DBusMessage *reply;
  DBusError error;
  const char *s;

  m_hasAdapter = false;
  dbus_error_init(&error);
  reply = message.SendSystem();
  if (reply != NULL)
  {
    dbus_message_get_args(reply, &error, DBUS_TYPE_OBJECT_PATH, &s, DBUS_TYPE_INVALID);
    if (dbus_error_is_set(&error))
    {
      CLog::Log(LOGERROR, "Bluetooth: DBus Error %s - %s", error.name, error.message);
    }
    else
    {
      m_adapter = s;
      m_hasAdapter = true;
      CLog::Log(LOGDEBUG, "Bluetooth: using adapter %s", s);
    }
  }
}

void CBluezBluetoothSyscall::StartDiscovery()
{
  if (m_hasAdapter)
  {
    CDBusMessage message("org.bluez", m_adapter.c_str(), "org.bluez.Adapter", "StartDiscovery");
    message.Send(m_connection);
    m_agent = new CBluezBluetoothAgent(m_connection);
  }
}

void CBluezBluetoothSyscall::StopDiscovery()
{
  if (m_hasAdapter)
  {
    CDBusMessage message("org.bluez", m_adapter.c_str(), "org.bluez.Adapter", "StopDiscovery");
    message.Send(m_connection);
    delete m_agent;
    m_agent = NULL;
  }
}

void CBluezBluetoothSyscall::CreateDevice(const char *address)
{
  if (m_hasAdapter && m_agent != NULL)
  {
    CDBusMessage message("org.bluez", m_adapter.c_str(), "org.bluez.Adapter", "CreatePairedDevice");
    message.AppendArgument(address);
    message.AppendObjectPath(m_agent->GetPath());
    message.AppendArgument("DisplayYesNo");
    message.SendAsync(m_connection);
  }
}

void CBluezBluetoothSyscall::RemoveDevice(const char *id)
{
  if (m_hasAdapter)
  {
    CDBusMessage message("org.bluez", m_adapter.c_str(), "org.bluez.Adapter", "RemoveDevice");
    message.AppendObjectPath(id);
    message.SendAsync(m_connection);
  }
}

void CBluezBluetoothSyscall::ConnectDevice(const char *id)
{
  if (m_hasAdapter)
  {
    CBluezBluetoothDevice device(id);
    switch (device.GetDeviceType())
    {
    case IBluetoothDevice::DEVICE_TYPE_HEADSET:
    case IBluetoothDevice::DEVICE_TYPE_HEADPHONES:
    case IBluetoothDevice::DEVICE_TYPE_AUDIO:
      {
        CDBusMessage message("org.bluez", id, "org.bluez.Audio", "Connect");
        message.SendAsync(m_connection);
      }
      break;
    default:
      {
        CDBusMessage message("org.bluez", id, "org.bluez.Input", "Connect");
        message.SendAsync(m_connection);
      }
      break;
    }
  }
}

void CBluezBluetoothSyscall::DisconnectDevice(const char *id)
{
  if (m_hasAdapter)
  {
    CDBusMessage message("org.bluez", id, "org.bluez.Device", "Disconnect");
    message.SendAsync(m_connection);
  }
}

bool CBluezBluetoothSyscall::PumpBluetoothEvents(IBluetoothEventsCallback *callback)
{
  bool result = false;
  if (m_hasAdapter && m_connection)
  {
    dbus_connection_read_write(m_connection, 0);
    DBusMessage *msg = dbus_connection_pop_message(m_connection);

    if (msg)
    {
      DBusMessageIter iter;
      dbus_message_iter_init(msg, &iter);
      result = true;
      if (dbus_message_is_signal(msg, "org.bluez.Adapter", "DeviceFound"))
      {
        char *address = NULL;
        dbus_message_iter_get_basic(&iter, &address);
        dbus_message_iter_next(&iter);
        CVariant properties = CDBusUtil::ParseDictionary(&iter);
        CBluezBluetoothDevice device(address, properties["Name"].asString(),
          (uint32_t)properties["Class"].asUnsignedInteger());
        CLog::Log(LOGDEBUG, "Bluetooth: found new device %s (%s)", address, device.GetName());
        callback->OnDeviceFound(&device);
      }
      else if (dbus_message_is_signal(msg, "org.bluez.Adapter", "DeviceDisappeared"))
      {
        char *address = NULL;
        dbus_message_iter_get_basic(&iter, &address);
        CLog::Log(LOGDEBUG, "Bluetooth: device %s disappeared", address);
        callback->OnDeviceDisappeared(address);
      }
      else if (dbus_message_is_signal(msg, "org.bluez.Adapter", "DeviceCreated"))
      {
        char *object = NULL;
        if (dbus_message_get_args(msg, NULL, DBUS_TYPE_OBJECT_PATH, &object, DBUS_TYPE_INVALID))
        {
          CBluezBluetoothDevice device(object);
          CLog::Log(LOGDEBUG, "Bluetooth: created device %s (%s, %s)", object, device.GetName(), device.GetAddress());

          // Always trust the device once paired
          CDBusMessage message("org.bluez", object, "org.bluez.Device", "SetProperty");
          message.AppendArgument("Trusted");
          message.AppendVariant(true);
          message.Send(m_connection);

          callback->OnDeviceCreated(&device);
        }
      }
      else if (dbus_message_is_signal(msg, "org.bluez.Adapter", "DeviceRemoved"))
      {
        char *object = NULL;
        if (dbus_message_get_args(msg, NULL, DBUS_TYPE_OBJECT_PATH, &object, DBUS_TYPE_INVALID))
        {
          CLog::Log(LOGDEBUG, "Bluetooth: removed device %s", object);
          callback->OnDeviceRemoved(object);
        }
      }
      else if (dbus_message_is_signal(msg, "org.bluez.Device", "PropertyChanged"))
      {
        char *name = NULL;
        dbus_message_iter_get_basic(&iter, &name);
        const char *path = dbus_message_get_path(msg);
        if (strcmp(name, "Connected") == 0)
        {
          CBluezBluetoothDevice device(path);
          if (device.IsConnected())
          {
            CLog::Log(LOGDEBUG, "Bluetooth: connected: %s (%s)", device.GetID(), device.GetName());
            callback->OnDeviceConnected(&device);
          }
          else
          {
            CLog::Log(LOGDEBUG, "Bluetooth: disconnected: %s (%s)", device.GetID(), device.GetName());
            callback->OnDeviceDisconnected(&device);
          }
        }
        else if (strcmp(name, "Paired") == 0)
        {
          CBluezBluetoothDevice device(path);
          if (device.IsPaired())
          {
            CLog::Log(LOGDEBUG, "Bluetooth: paired: %s (%s)", device.GetID(), device.GetName());
            // Connect the device once paired.
            CGUIDialogOK *dialog = (CGUIDialogOK *)g_windowManager.GetWindow(WINDOW_DIALOG_OK);
            if (dialog)
              dialog->Close();
            ConnectDevice(path);
          }
        }
      }
      else if (m_agent != NULL && m_agent->ProcessMessage(msg))
      {
      }
      else if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR)
      {
        CLog::Log(LOGERROR, "Bluetooth: received error %s", dbus_message_get_error_name(msg));
      }
      else
      {
        CLog::Log(LOGDEBUG, "Bluetooth: ignored message, member=%s.%s, type=%s, object=%s",
          dbus_message_get_interface(msg), dbus_message_get_member(msg),
          dbus_message_type_to_string(dbus_message_get_type(msg)),
          dbus_message_get_path(msg));
      }
      dbus_message_unref(msg);
    }
  }
  return result;
}


#endif

