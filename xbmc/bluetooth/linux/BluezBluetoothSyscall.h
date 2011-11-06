#pragma once

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


#ifdef HAS_DBUS

#include "bluetooth/IBluetoothSyscall.h"
#include "DBusUtil.h"
#include "DBusAgent.h"

class CBluezBluetoothDevice : public IBluetoothDevice
{
public:
  CBluezBluetoothDevice(const char* id);
  CBluezBluetoothDevice(const char* address, const char *name, uint32_t cls);
  const char* GetID() { return m_id.c_str(); }
  const char* GetName() { return m_name.c_str(); }
  const char* GetAddress() { return m_address.c_str(); }
  bool IsConnected() { return m_connected; }
  bool IsCreated() { return m_created; }
  bool IsPaired() { return m_paired; }
  DeviceType GetDeviceType() { return m_deviceType; }
private:
  std::string m_id;
  std::string m_name;
  std::string m_address;
  bool m_connected;
  bool m_created;
  bool m_paired;
  DeviceType m_deviceType;
  DeviceType GetDeviceTypeFromClass(uint32_t cls);
};

class CBluezBluetoothAgent : public CDBusAgent<CBluezBluetoothAgent>
{
public:
  CBluezBluetoothAgent(DBusConnection *conn);
protected:
  void Release(std::vector<CVariant> &args);
  void RequestPinCode(std::vector<CVariant> &args);
  void RequestPasskey(std::vector<CVariant> &args);
  void DisplayPasskey(std::vector<CVariant> &args);
  void RequestConfirmation(std::vector<CVariant> &args);
  void Authorize(std::vector<CVariant> &args);
  void ConfirmModeChange(std::vector<CVariant> &args);
  void Cancel(std::vector<CVariant> &args);
};

class CBluezBluetoothSyscall : public IBluetoothSyscall
{
public:
  CBluezBluetoothSyscall();
  ~CBluezBluetoothSyscall();
  std::vector<boost::shared_ptr<IBluetoothDevice> > GetDevices();
  boost::shared_ptr<IBluetoothDevice> GetDevice(const char *id);
  void StartDiscovery();
  void StopDiscovery();
  void CreateDevice(const char *address);
  void RemoveDevice(const char *id);
  void ConnectDevice(const char *id);
  void DisconnectDevice(const char *id);
  bool PumpBluetoothEvents(IBluetoothEventsCallback *callback);

private:
  std::string m_adapter;
  bool m_hasAdapter;
  DBusConnection *m_connection;
  CBluezBluetoothAgent *m_agent;

  void SetupDefaultAdapter();
};

#endif
