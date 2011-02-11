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

#include "IBluetoothSyscall.h"

class CNullBluetoothSyscall : public IBluetoothSyscall
{
public:
  std::vector<boost::shared_ptr<IBluetoothDevice> > GetDevices() { return std::vector<boost::shared_ptr<IBluetoothDevice> >(); }
  boost::shared_ptr<IBluetoothDevice> GetDevice(const char *id) { return boost::shared_ptr<IBluetoothDevice>(); }
  void StartDiscovery() { }
  void StopDiscovery() { }
  void CreateDevice(const char *address) { }
  void RemoveDevice(const char *id) { }
  void ConnectDevice(const char *id) { }
  void DisconnectDevice(const char *id) { }
  bool PumpBluetoothEvents(IBluetoothEventsCallback *callback) { return false; }
};

class CBluetoothManager : public IBluetoothEventsCallback
{
public:
  CBluetoothManager();
  ~CBluetoothManager();

  void Initialize();

  std::vector<boost::shared_ptr<IBluetoothDevice> > GetDevices();
  boost::shared_ptr<IBluetoothDevice> GetDevice(const char *id);
  void StartDiscovery();
  void StopDiscovery();
  void CreateDevice(const char *address);
  void RemoveDevice(const char *id);
  void ConnectDevice(const char *id);
  void DisconnectDevice(const char *id);
  void ProcessEvents();

private:
  void OnDeviceConnected(IBluetoothDevice *device);
  void OnDeviceDisconnected(IBluetoothDevice *device);
  void OnDeviceFound(IBluetoothDevice *device);
  void OnDeviceDisappeared(const char *address);
  void OnDeviceCreated(IBluetoothDevice *device);
  void OnDeviceRemoved(const char *id);

  IBluetoothSyscall *m_instance;
};

extern CBluetoothManager g_bluetoothManager;

