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

#include <string>
#include <vector>

class IBluetoothDevice
{
public:
  enum DeviceType
  {
    DEVICE_TYPE_UNKNOWN = 0,
    DEVICE_TYPE_MOUSE,
    DEVICE_TYPE_KEYBOARD,
    DEVICE_TYPE_HEADSET,
    DEVICE_TYPE_HEADPHONES,
    DEVICE_TYPE_AUDIO,
    DEVICE_TYPE_PHONE,
    DEVICE_TYPE_COMPUTER,
    DEVICE_TYPE_MODEM,
    DEVICE_TYPE_NETWORK,
    DEVICE_TYPE_VIDEO,
    DEVICE_TYPE_JOYPAD,
    DEVICE_TYPE_TABLET,
    DEVICE_TYPE_PRINTER,
    DEVICE_TYPE_CAMERA
  };

  virtual ~IBluetoothDevice() { }
  virtual const char* GetID()        = 0;
  virtual const char* GetName()      = 0;
  virtual const char* GetAddress()   = 0;
  virtual bool IsConnected()         = 0;
  virtual bool IsCreated()           = 0;
  virtual bool IsPaired()            = 0;
  virtual DeviceType GetDeviceType() = 0;
};

class IBluetoothEventsCallback
{
public:
  virtual ~IBluetoothEventsCallback() { }
  virtual void OnDeviceConnected(IBluetoothDevice *device)    = 0;
  virtual void OnDeviceDisconnected(IBluetoothDevice *device) = 0;
  virtual void OnDeviceFound(IBluetoothDevice *device)        = 0;
  virtual void OnDeviceDisappeared(const char *address)       = 0;
  virtual void OnDeviceCreated(IBluetoothDevice *device)      = 0;
  virtual void OnDeviceRemoved(const char *id)                = 0;
};

class IBluetoothSyscall
{
public:
  virtual ~IBluetoothSyscall() { }
  virtual std::vector<boost::shared_ptr<IBluetoothDevice> > GetDevices() = 0;
  virtual boost::shared_ptr<IBluetoothDevice> GetDevice(const char *id) = 0;
  virtual void StartDiscovery()                         = 0;
  virtual void StopDiscovery()                          = 0;
  virtual void CreateDevice(const char *address)        = 0;
  virtual void RemoveDevice(const char *id)             = 0;
  virtual void ConnectDevice(const char *id)            = 0;
  virtual void DisconnectDevice(const char *id)         = 0;

  /*!
   \brief Pump bluetooth related events back to xbmc.

   PumpPowerEvents is called from Application Thread and the platform implementation may signal
   power related events back to xbmc through the callback.

   return true if an event occured and false if not.

   \param callback the callback to signal to
   */
  virtual bool PumpBluetoothEvents(IBluetoothEventsCallback *callback) = 0;
};

